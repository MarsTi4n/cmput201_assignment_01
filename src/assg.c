#include <stdio.h>
#include <string.h>

#define max_str_len 100
#define num_schedules 221

typedef char string[max_str_len + 1];


int count_schedules(FILE *fp, string schedules[])
{
    int count = 0;

    while (count < num_schedules && fgets(schedules[count], sizeof schedules[count], fp) != NULL) 
    {
        size_t end = strcspn(schedules[count], "\r\n");
        int has_newline = schedules[count][end] != '\0';

        schedules[count][end] = '\0';

        /* If a line exceeds 40 characters, discard its remainder. */
        if (!has_newline) {
            int ch = fgetc(fp);
            while (ch != '\n' && ch != EOF) {
                ch = fgetc(fp);
            }
        }
        count++;
    }

    for (int i = 0; i < count; i++) {
        string temp = "../";
        strcat(temp, schedules[i]);
        FILE *temp_fp = fopen(temp, "r");
        if (temp_fp == NULL) {
            printf("Error opening schedule file\n\n");
            continue;
        }

        char dir;
        char *direction = strrchr(schedules[i], '_');
        if (direction != NULL && direction[1] != '\0') {
            dir = direction[1];  /* 'E' for EB, 'W' for WB */
        }

        string count_schedule_header; 
        fgets(count_schedule_header, sizeof schedules[i], temp_fp);
        //printf(count_schedule_header);

        fclose(temp_fp);

        int schedule_num, num_stops;
        string route_ID;

        if (sscanf(count_schedule_header, "%d %40[^,], %d Stops", &schedule_num, route_ID, &num_stops) == 3) {
            for (int i = 0; route_ID[i] != '\0'; i++) {
                if (route_ID[i] == '_') {
                    route_ID[i] = ' ';
                }
            }
        }  
        //printf("Route num: %d, Schedule: %s, Number of Stops: %d, Direction: %c\n\n", schedule_num, route_ID, num_stops, dir);
        char schedule_str[100];
        sprintf(schedule_str, "is route %d it has %d stops and is in the %c direction", schedule_num, num_stops, dir);
        //printf("%s\n\n", schedule_str);
        snprintf(schedules[i], sizeof schedules[i], "%s", schedule_str);    }

    return count;
}


int main(void)
{
    string schedules[num_schedules];

    FILE *fp = fopen("list_file.txt", "r");
    if (fp == NULL) {
        printf("Error opening list_file.txt");
        return 1;
    }

    int count = count_schedules(fp, schedules);
    fclose(fp);

    FILE *output_fp = fopen("output.txt", "a");

    //PRINTING SECTION
    fprintf(output_fp, "Processing %d schedule files. . .\n", count);
    for (int i = 0; i < count; i++) {
        fprintf(output_fp, "schedule #%d %s\n", i + 1, schedules[i]);
    }
    fclose(output_fp);


    return 0;
}