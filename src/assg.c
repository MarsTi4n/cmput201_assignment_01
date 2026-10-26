#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define max_str_len 100

typedef char string[max_str_len + 1];

typedef struct {
    string date;
    string time;
    string type;
    string location;
} Entry;

int is_digit(char ch) {
    return ch >= '0' && ch <= '9';
}

int valid_date(const char date[]) {
    if (strlen(date) != 10 || date[2] != '-' || date[5] != '-') {
        return 0;
    }
    for (int i = 0; i < 10; i++) {
        if (i != 2 && i != 5 && !is_digit(date[i])) {
            return 0;
        }
    }
    return 1;
}

int valid_time(const char time[]) {
    return strlen(time) == 5 && time[2] == ':' &&
           is_digit(time[0]) && is_digit(time[1]) &&
           is_digit(time[3]) && is_digit(time[4]);
}

int valid_type(const char type[]) {
    return strlen(type) == 1 && type[0] >= 'A' && type[0] <= 'Z';
}

int valid_location(const char location[]) {
    if (strlen(location) != 4) {
        printf("Invalid Stop ID\n");
        return 0;
    }
    for (int i = 0; i < 4; i++) {
        if (!is_digit(location[i])) {
            printf("Invalid Stop ID\n");
            return 0;
        }
    }
    return 1;
}

int read_entries(FILE *input, Entry entries[], int max_entries, FILE *output) {
    char line[4 * (max_str_len + 1)];
    int count = 0;

    while (count < max_entries && fgets(line, sizeof line, input) != NULL) {
        char ampm[3];
        int hour, minute, day, month, year;

        int fields = sscanf(line, " %10[^,], %5[0-9:] %2s, %100[^,], %100[^\r\n]",
                   entries[count].date, entries[count].time, ampm,
                   entries[count].type, entries[count].location);
        
        if (fields <= 4) {

            if (fields == 4 && entries[count].type[0] == 'M' &&
                entries[count].type[1] == '\0') {
                strcpy(entries[count].location, "NULL");
            }
            else{
                printf("Invalid data format\n");
                return -1;
            }
        }
        if (sscanf(entries[count].time, "%d:%d", &hour, &minute) != 2 ||
            hour < 1 || hour > 12 || minute < 0 || minute > 59 ||
            sscanf(entries[count].date, "%2d-%2d-%4d", &month, &day, &year) != 3 ||
            day < 1 || day > 31 || month < 1 || month > 12 || year < 0 ||
            (strcmp(ampm, "AM") != 0 && strcmp(ampm, "PM") != 0)) {
                return -1;
            }
        if (strcmp(entries[count].location, "NULL") != 0 &&
             !valid_location(entries[count].location)){
                return -1;
             }
        if (!valid_date(entries[count].date) || !valid_time(entries[count].time) 
         || !valid_type(entries[count].type)) {
            printf("Invalid data format\n");
            return -1;
        }

        if (strcmp(ampm, "AM") == 0) {
            hour %= 12;
        } 
        else {
            hour = hour % 12 + 12;
        }
        snprintf(entries[count].time, sizeof entries[count].time,
                 "%02d:%02d:00", hour, minute);
        snprintf(entries[count].date, sizeof entries[count].date,
                 "%04d-%02d-%02d", year, month, day);
        count++;
    }

    return count;
}

int schedule_has_stops(const char path[], const char first_stop[], const char second_stop[]) {
    FILE *schedule = fopen(path, "r");
    char line[256];
    int found_first = 0;
    int found_second = 0;

    if (schedule == NULL) {
        return 0;
    }

    while (fgets(line, sizeof line, schedule) != NULL) {
        char *separator;
        line[strcspn(line, "\r\n")] = '\0';
        separator = strchr(line, ',');
        if (separator != NULL) {
            *separator = '\0';
            if (strcmp(line, first_stop) == 0) {
                found_first = 1;
            }
            if (strcmp(line, second_stop) == 0) {
                found_second = 1;
            }
        }
    }

    fclose(schedule);
    return found_first && found_second;
}

char *find_stop_name(const char location[], FILE *list_file) {
    char schedule_path[256];
    char line[512];

    rewind(list_file);
    while (fgets(schedule_path, sizeof schedule_path, list_file) != NULL) {
        FILE *schedule;

        schedule_path[strcspn(schedule_path, "\r\n")] = '\0';
        schedule = fopen(schedule_path, "r");
        if (schedule == NULL) {
            continue;
        }

        while (fgets(line, sizeof line, schedule) != NULL) {
            char *separator;
            size_t name_length;

            line[strcspn(line, "\r\n")] = '\0';
            separator = strchr(line, ',');
            if (separator == NULL) {
                continue;
            }
            *separator = '\0';
            if (strcmp(line, location) == 0) {
                char *name = separator + 1;
                while (*name == ' ' || *name == '\t') {
                    name++;
                }
                name_length = strlen(name);
                char *result = malloc(name_length + 1);
                if (result != NULL) {
                    memcpy(result, name, name_length + 1);
                }
                fclose(schedule);
                rewind(list_file);
                return result;
            }
        }

        fclose(schedule);
    }

    rewind(list_file);
    return NULL;
}

char *get_duration(Entry start, Entry exit) {
    static char difference[16];
    int start_hour, start_minute, start_second;
    int exit_hour, exit_minute, exit_second;

    sscanf(start.time, "%d:%d:%d", &start_hour, &start_minute, &start_second);
    sscanf(exit.time, "%d:%d:%d", &exit_hour, &exit_minute, &exit_second);

    int seconds = (exit_hour * 3600 + exit_minute * 60 + exit_second) -
                  (start_hour * 3600 + start_minute * 60 + start_second);

    snprintf(difference, sizeof difference, "%01d:%02d:%02d",
            seconds / 3600,
             seconds / 60 % 60, seconds % 60);
    return difference;
}

void routes_matching(Entry entries[], int num_entries, FILE *list_file, FILE *output) {
    if (num_entries % 2 != 0) {
        printf("Invalid number of entries\n");
        return;
    }

    char schedule_path[256];
    int schedule_count = 0;
    rewind(list_file);
    while (fgets(schedule_path, sizeof schedule_path, list_file) != NULL) {
        schedule_count++;
    }

    size_t route_capacity = (size_t)(schedule_count > 0 ? schedule_count : 1);
    char (*valid_schedules)[256] = malloc(route_capacity * sizeof *valid_schedules);
    size_t trip_count = (size_t)num_entries / 2;
    char (*selected_schedules)[256] = calloc(trip_count > 0 ? trip_count : 1,
                                               sizeof *selected_schedules);
    char (*guessed_locations)[max_str_len + 1] = calloc(
        trip_count > 0 ? trip_count : 1, sizeof *guessed_locations);
    if (valid_schedules == NULL || selected_schedules == NULL ||
        guessed_locations == NULL) {
        free(valid_schedules);
        free(selected_schedules);
        free(guessed_locations);
        fprintf(output, "Unable to allocate route storage\n");
        return;
    }

    for (int i = num_entries-1; i > 0; i-=2) {
        if (strcmp(entries[i].type, "E") == 0 &&
            (strcmp(entries[i - 1].type, "X") == 0 ||
             strcmp(entries[i - 1].type, "M") == 0)) {
            const char *exit_location = entries[i - 1].location;
            int guessed = strcmp(entries[i - 1].type, "M") == 0;
            int matching_route_count = 0;
            size_t trip_index = (size_t)(num_entries - 1 - i) / 2;

            if (guessed) {
                for (int previous_exit = i + 1;
                     previous_exit + 1 < num_entries; previous_exit += 2) {
                    if (strcmp(entries[previous_exit].type, "X") == 0 &&
                        strcmp(entries[previous_exit + 1].type, "E") == 0 &&
                        strcmp(entries[previous_exit + 1].location,
                               entries[i].location) == 0) {
                        exit_location = entries[previous_exit].location;
                        break;
                    }
                }
                if (strcmp(exit_location, "NULL") == 0) {
                    continue;
                }
                strcpy(guessed_locations[trip_index], exit_location);
            }

            rewind(list_file);
            while (fgets(schedule_path, sizeof schedule_path, list_file) != NULL) {
                schedule_path[strcspn(schedule_path, "\r\n")] = '\0';
                if (schedule_has_stops(schedule_path, entries[i].location,
                                       exit_location)) {
                    if ((size_t)matching_route_count >= route_capacity) {
                        break;
                    }
                    strcpy(valid_schedules[matching_route_count], schedule_path);
                    matching_route_count++;
                }
            }

            if (matching_route_count > 1) {
                printf("Ambiguous trip detected\n");
                char *entry_name = find_stop_name(entries[i].location, list_file);
                char *exit_name = find_stop_name(exit_location, list_file);
                printf("Entry: %s (%s)\nExit: %s (%s)\n",
                       entries[i].location, entry_name,
                       entries[i-1].location, exit_name);
                free(entry_name);
                free(exit_name);
                printf("Possible schedules:\n");
                for (int j = 0; j < matching_route_count; j++) {
                    char route_number[100];
                    char route_direction[100];
                    char *filename;

                    filename = strrchr(valid_schedules[j], '/');
                    filename = filename == NULL ? valid_schedules[j] : filename + 1;
                    if (sscanf(filename, "route_%99[^_]_%1[^.].txt", route_number,
                               route_direction) == 2) {
                        printf("%d. %s %s\n", j + 1, route_number,
                               route_direction);
                    }
                }
                printf("Select route (1-%d): ", matching_route_count);
                int choice;
                while(scanf("%d", &choice) != 1 || choice < 1 || choice > matching_route_count) {
                    printf("Invalid choice. Please select a valid route (1-%d): ", matching_route_count);
                    while(getchar() != '\n'); // Clear input buffer
                }
                strcpy(selected_schedules[trip_index], valid_schedules[choice - 1]);
            } else if (matching_route_count == 1) {
                strcpy(selected_schedules[trip_index], valid_schedules[0]);
            }
        }
    }
    
    for (size_t i = 0; i < trip_count; i++) {
        int entry_index = num_entries - 1 - (int)(i * 2);
        char route_number[100];
        char route_direction[100];
        char *filename;

        if (selected_schedules[i][0] == '\0' || entry_index <= 0) {
            continue;
        }
        filename = strrchr(selected_schedules[i], '/');
        filename = filename == NULL ? selected_schedules[i] : filename + 1;

        if (sscanf(filename, "route_%99[^_]_%1[^.].txt", route_number,
                   route_direction) == 2) {
            char *duration = get_duration(entries[entry_index], entries[entry_index - 1]);
            char *start_stop = find_stop_name(entries[entry_index].location, list_file);
            const char *end_location = entries[entry_index - 1].type[0] == 'M'
                ? guessed_locations[i] : entries[entry_index - 1].location;
            char *end_stop = find_stop_name(end_location, list_file);
            fprintf(output, "%s %s,%s,%s,%sT%s,%sT%s,%s%s\n",
                    route_number, route_direction,
                    start_stop,end_stop,
                    entries[entry_index].date, entries[entry_index].time,
                    entries[entry_index - 1].date,
                    entries[entry_index - 1].time,
                    duration,
                    entries[entry_index - 1].type[0] == 'M' ? ",Guessed" : "");
            free(start_stop);
            free(end_stop);
        }
    }

    free(valid_schedules);
    free(selected_schedules);
    free(guessed_locations);
}

int main(int argc, char *argv[])
{
    FILE *output = fopen("output.txt", "w");
    FILE *fp = fopen(argv[1], "r");
    FILE *input = fopen(argv[2], "r");
    
    if (argc > 2) {
        // printf("Usage: %s %s %s\n", argv[0], argv[1], argv[2]);
    }
    else{
        fprintf(output, "missing arguments Usage: %s\n", argv[0]);
        return 1;
    }
    if (fp == NULL || input == NULL) {
        fprintf(output, "Error opening file(s)\n");
        fclose(fp);
        fclose(input);
        fclose(output);
        return 1;
    }


    int num_entries = 0;
    fscanf(input, "%d", &num_entries);  
    fgetc(input);

    int capacity = num_entries > 0 ? num_entries : 1;
    Entry entries[capacity];
    num_entries = read_entries(input, entries, num_entries, output);
    if (num_entries < 0) {
        fclose(fp);
        fclose(input);
        fclose(output);
        return 1;
    }



    //PRINTING TO FILE SECTION

    routes_matching(entries, num_entries, fp, output);

    fclose(fp);
    fclose(input);
    fclose(output);

    return 0;
}

