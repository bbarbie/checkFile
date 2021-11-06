/**
 * @file main.c
 * @date 2021-10-01
 * @author Bruna Leal 2201732
 * @author Tomás Pereira 2201785
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <errno.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <assert.h>
#include <stdint.h>
#include <dirent.h>
#include <signal.h>
#include <ctype.h>
#include <time.h>
#include <libgen.h>

#include "debug.h"
#include "memory.h"
#include "args.h"



void help();
int isRegularFile(const char *fileName);
void checkFile(const char *fileName);


int main(int argc, char *argv[]) {

    (void)argc;
    (void)argv; 

    struct gengetopt_args_info args;
	if (cmdline_parser(argc,argv,&args) != 0){
	exit(1);
	}

    int option = 0;

    if (args.file_given){

        printf("[INFO] analyzing file/s\n");

        for(unsigned int i = 0; i < args.file_given; i++){

            FILE *file = fopen(args.file_arg[i], "r");
            if (file == NULL) 
            {
                ERROR(1, "Error opening file '%s'\n", args.file_arg[i]);
            }

            checkFile(args.file_arg[i]);
            
            fclose(file);
		}

	} else if (args.batch_given){

        printf("[INFO] analyzing files listed in '%s'", args.batch_arg);

		FILE *file = fopen(args.batch_arg, "r");
            if (file == NULL) 
            {
                ERROR(1, "Error opening file '%s'\n", args.batch_arg);
            }

            char *line;
            size_t len = 0;
            while (getline (&line, &len, file) != -1){

                printf("%s", line);
                checkFile(line);
            }

            fclose(file);
                  
	} else if (args.dir_given){

        printf("[INFO] analyzing files of directory '%s'", args.dir_arg);

        struct dirent *dp;

        DIR *dir = opendir(args.dir_arg);
        if (!dir){ 
            ERROR(1,"Unable to open director"); 
        }

        while ((dp = readdir(dir)) != NULL){

            char* fileName = dp->d_name;
            printf("%s\n", fileName);
            checkFile(fileName);

        }

        closedir(dir);
    
        
    } else if (args.help_given){

        help();

    }

    return 0;
}


void help() {
    printf("CheckFile will check if the file extension is the real file type or not\n");
    printf("Command format: ./checkFile -op [argument]\n");
    printf("Program options:\n-f -> one or more files\n-b -> file with one or more file names/paths\n-d -> directory name/path\n-h -> help manual\n");
    printf("Author 1: Bruna Bastos LeaL -- 2201732\n");
    printf("Author 2: Tomás Vilhena Pereira -- 2201785\n");
    printf("File extensions supported by program: \nPDF, GIF, JPG, PNG, MP4, ZIP, HTML\n");
    exit(0);
}

int isRegularFile(const char *fileName){

    const char ch = '.';
    char *ret;
    ret = strrchr(fileName, ch);

    if(ret == NULL || (strcmp(ret, "..") == 0) || (strcmp(ret, ".") == 0)){
        printf("[INFO] not a file\n");
        return -1;

    } else if((strcmp(ret, ".pdf") != 0) && (strcmp(ret, ".gif") != 0) && (strcmp(ret, ".jpg") != 0)
               && (strcmp(ret, ".png") != 0) && (strcmp(ret, ".mp4") != 0) && (strcmp(ret, ".zip") != 0)
               && (strcmp(ret, ".html") != 0)){

        printf("[INFO] '%s' is not supported by checkFile\n", fileName);
        return -1;

    }else {
        printf("Valid file type\n");
        return 0;
    }

}



void checkFile(const char *fileName){

    int regular = isRegularFile(fileName);

    //create temporary file
    FILE* tmpFile = tmpfile(); 
    int fd = fileno(tmpFile); //get the file descriptor from an open stream
    char* token;

    if(regular == 0){

        pid_t pid = fork();
        if (pid < 0){
            ERROR(1, "[ERROR] failed to fork()");
        } else if(pid == 0){
            dup2(fd, 1); //redirect the output (stdout) to temporary file
            int execReturn = execl("/bin/file", "file", fileName, "--mime-type", "-b", NULL);
            if(execReturn == -1){
                ERROR(1, "[ERROR] failed to execl()");
            }
        } else{
            wait(NULL);

            const char ch = '.';
            char *ret;
            ret = strrchr(fileName, ch);
            ret++;

            char* fileExt = ret; //file extention after '.'

            // read til EOF and count nº of bytes
            fseek(tmpFile, 0, SEEK_END);
            long fsize = ftell(tmpFile)-1;
            fseek(tmpFile, 0, SEEK_SET); 
            char *string = malloc(fsize*sizeof(char)); 
            fread(string, 1, fsize, tmpFile);
            string[fsize] = 0; //Set the last '\n' byte to \0.
            fclose(tmpFile);

            char* strType = strrchr(string, '/');
            char* type = strType;

            if(strcmp(type, "x-empty") == 0){
                printf("[INFO] '%s': is an empty file\n", fileName);
                free(strType);
                return;
            }

            //jpg apeears as jpeg in exec the others still the same
            char* fullExt = fileExt;
            if(strcmp(fileExt, "jpg")==0){
                fullExt = "jpeg";
            }

            if(strcmp(type, fullExt)==0){
                printf("[OK] '%s': extension '%s' matches file type '%s'\n", fileName, fileExt, type);
            }else{
                printf("[MISMATCH] '%s': extension is '%s', file type is '%s'\n", fileName, fileExt, type);
            }
        }
    }
}


