#include <stdio.h>
#include <string.h>

#define max_str_len 100
#define num_schedules 221

typedef char string[max_str_len + 1];
int count_schedules(FILE *fp, string schedules[]);

int main(int argc, char *argv[])
{
    string schedules[num_schedules];

    if (argc > 1) {
        printf("Usage: %s %s\n", argv[0], argv[1]);
    }
    else{
        printf("no argv[1] Usage: %s\n", argv[0]);
    }

    FILE *fp = fopen(argv[1], "r");
    if (fp == NULL) {
        printf("Error opening list file\n");
        return 1;
    }

    int count = count_schedules(fp, schedules);
    fclose(fp);



    //PRINTING TO FILE SECTION
    fprintf(stdout, "Processing %d schedule files. . .\n", count);
    for (int i = 0; i < count; i++) {
        fprintf(stdout, "schedule #%d %s\n", i + 1, schedules[i]);
    }


    return 0;
}


int count_schedules(FILE *fp, string schedules[])
{
    int count = 0;

    while (count < num_schedules && fgets(schedules[count], sizeof schedules[count], fp) != NULL) 
    {
        size_t end = strcspn(schedules[count], "\r\n");
        int has_newline = schedules[count][end] != '\0';

        schedules[count][end] = '\0';

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
        char schedule_str[100];
        sprintf(schedule_str, "is route %d it has %d stops and is in the %c direction", schedule_num, num_stops, dir);
        snprintf(schedules[i], sizeof schedules[i], "%s", schedule_str);    }

    return count;
}
