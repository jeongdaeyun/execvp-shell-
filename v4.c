#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <stdbool.h>
#include <dirent.h>
#include <sys/stat.h>
#include <pwd.h>
#include <grp.h>
#include <time.h>
#include <termios.h>
#include <stdbool.h>
#include <fcntl.h>
#include <limits.h>

#define MAX_COMMAND_LENGTH 100
#define MAX_ARGS 10

void run(char *args[], bool flag, int arg_count); // 프로세스 생성과 명령어 처리 함수
int print_cwd();                                  // shell에 현재 위치 출력 함수
int command_pwd();                                // pwd 명령어 처리 함수
int command_cd(char *directory);                  // cd 명령어 처리 함수
int command_ls();                                 // ls 명령어 처리 함수
int command_ls_l();                               // ls -l 명령어 처리 함수
void print_permissions(mode_t mode);              // ls -l 파일 정보 출력 함수
int redirection(char *args[], int arg_count);     // 표준 출력 변경 명령어 처리 함수

int main()
{

    char command[MAX_COMMAND_LENGTH];
    char *args[MAX_ARGS];

    while (1)
    {
        // 프롬프트 표시
        printf("my_shell:");
        // 현재 위치 출력
        print_cwd();
        printf("> ");

        // 조건 1.
        // fgets() 함수를 사용하여 사용자로부터 명령을 입력
        fgets(command, MAX_COMMAND_LENGTH, stdin);

        // 조건 2.
        // 개행 문자만 입력한 경우 마지막으로 jump
        if (strcmp(command, "\n") == 0)
            goto end_of_loop;

        // 개행 문자 제거
        command[strcspn(command, "\n")] = '\0';

        // strtok() 함수를 사용하여 명령과 옵션을 분리
        int arg_count = 0;
        char *token = strtok(command, " ");

        // 명령을 첫 번째로 저장
        args[arg_count++] = token;

        // 나머지는 옵션 및 인자로 저장
        while (token != NULL && arg_count < MAX_ARGS - 1)
        {
            token = strtok(NULL, " ");
            args[arg_count++] = token;
        }

        // execvp() 종료를 위한 NULL 추가
        args[arg_count] = NULL;

        // 조건 3.
        // strcmp() 함수를 사용하여 종료와 백그라운드 여부를 확인

        // 종료 조건
        if (strcmp(args[0], "exit") == 0)
        {
            printf("good bye!\n");
            break;
        }

        // 백그라운드 여부 확인
        // 마지막 인자가 "&"인 경우 flag를 true로 설정
        bool flag = false;
        if (arg_count > 1 && strcmp(args[arg_count - 2], "&") == 0)
        {
            flag = true;
            args[arg_count - 2] = NULL; // "&"를 인자로 사용하지 않도록 설정
            printf("It's background process.\n");
        }

        run(args, flag, arg_count);

    end_of_loop:;
    }

    return 0;
}

void run(char *args[], bool flag, int arg_count)
{
    // 조건 4.
    // fork()를 사용하여 자식 프로세스 생성
    pid_t pid = fork();

    if (pid == -1)
    {
        perror("fork");
        exit(EXIT_FAILURE);
    }
    // 자식 프로세스
    else if (pid == 0)
    {

        // 리다이렉션 처리
        if (redirection(args, arg_count) == -1)
        {
            exit(EXIT_FAILURE);
        }

        // cd 명령어 무시
        if (strcmp(args[0], "cd") == 0)
        {
            exit(EXIT_SUCCESS);
        }

        // 직접 구현한 명령어 처리
        // pwd 명령어 처리
        if (strcmp(args[0], "pwd") == 0)
        {
            if (command_pwd() == -1)
            {
                exit(EXIT_FAILURE);
            }
            exit(EXIT_SUCCESS);
        }

        // ls 명령어 처리
        else if ((strcmp(args[0], "ls") == 0) &&
                 (args[1] == NULL || strcmp(args[1], "-l") == 0))
        {
            // -l 옵션 처리
            if (args[1] != NULL && strcmp(args[1], "-l") == 0)
            {
                if (command_ls_l() == -1)
                {
                    exit(EXIT_FAILURE);
                }
                exit(EXIT_SUCCESS);
            }
            else
            {
                if (command_ls() == -1)
                {
                    exit(EXIT_FAILURE);
                }
                exit(EXIT_SUCCESS);
            }
        }

        // 위에 구현되지 않은 명령어들은
        // execvp() 함수를 사용해 실행
        else
        {
            if (execvp(args[0], args) == -1)
            {
                printf("command not found | invalid option\n");
                exit(EXIT_FAILURE);
            }
        }
    }
    // 부모 프로세스
    else
    {
        // 백그라운드 실행이 아닌 경우 자식 프로세스의 종료를 기다림
        if (!flag)
        {
            int status;
            waitpid(pid, &status, 0);
        }

        // cd 명령어 처리
        if (strcmp(args[0], "cd") == 0)
        {
            command_cd(args[1]);
        }
    }
}

// 위치 출력 함수
int print_cwd()
{
    char cwd[1024];

    if (getcwd(cwd, sizeof(cwd)) != NULL)
    {
        printf("%s", cwd);
    }
    else
    {
        perror("getcwd() error");
        return -1;
    }
    return 0;
}

// 표준 출력 변경 명령어 처리 함수
int redirection(char *args[], int arg_count)
{
    int fd, idx = -1;
    // '>' 문자를 찾고, 해당 인덱스 저장
    for (int i = 0; i < arg_count - 1; i++)
    {
        if (strcmp(args[i], ">") == 0)
        {
            idx = i;
            break;
        }
    }

    // '>' 문자가 없으면 리다이렉션 없음
    if (idx == -1)
    {
        return 0;
    }

    // '>' 뒤에 파일 이름이 없는 경우
    if (idx == arg_count - 1)
    {
        printf("No output file specified\n");
        return -1;
    }

    fd = open(args[idx + 1], O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (fd < 0)
    {
        perror("open");
        return -1;
    }

    // 표준 출력을 파일 디스크립터로 변경
    dup2(fd, STDOUT_FILENO);
    close(fd);

    // '>' 및 파일 이름 인자 제거
    args[idx] = NULL;
    return 0;
}

// cd 명령어 처리 함수
int command_cd(char *directory)
{
    if (chdir(directory) == -1)
    {
        perror("chdir");
        return -1;
    }
    return 0;
}

// pwd 명령어 처리 함수
int command_pwd()
{
    char cwd[1024];

    if (getcwd(cwd, sizeof(cwd)) != NULL)
    {
        printf("%s\n", cwd);
    }
    else
    {
        perror("getcwd() error");
        return -1;
    }
    return 0;
}

// ls 명령어 처리 함수
int command_ls()
{
    struct dirent *de;      // 디렉토리 엔트리 포인터
    DIR *dr = opendir("."); // 현재 디렉토리 열기

    if (dr == NULL)
    { // 디렉토리를 열 수 없는 경우
        printf("Could not open current directory");
        return -1;
    }

    while ((de = readdir(dr)) != NULL)
    {
        printf("%s  ", de->d_name); // 디렉토리의 내용을 출력
    }
    printf("\n");
    closedir(dr); // 디렉토리 닫기
    return 0;
}

int command_ls_l()
{
    struct dirent *de;
    struct stat st;
    DIR *dr = opendir(".");
    char timebuf[256];
    struct tm *tm_info;
    long total_blocks = 0;

    if (dr == NULL)
    {
        printf("Could not open current directory");
        return -1;
    }
    while ((de = readdir(dr)) != NULL)
    {
        if (stat(de->d_name, &st) == 0)
        {
            total_blocks += st.st_blocks;
        }
    }
    rewinddir(dr);
    // 총 사용량 출력
    printf("total %ld\n", total_blocks / 2);

    while ((de = readdir(dr)) != NULL)
    {
        stat(de->d_name, &st);
        print_permissions(st.st_mode);
        printf(" %u", st.st_nlink);
        printf(" %s", getpwuid(st.st_uid)->pw_name);
        printf(" %s", getgrgid(st.st_gid)->gr_name);
        printf(" %5ld", st.st_size);
        tm_info = localtime(&st.st_mtime);
        strftime(timebuf, 26, "%Y-%m-%d %H:%M", tm_info);
        printf(" %s", timebuf);
        printf(" %s\n", de->d_name);
    }

    closedir(dr);
    return 0;
}

void print_permissions(mode_t mode)
{
    printf((S_ISDIR(mode)) ? "d" : "-");
    printf((mode & S_IRUSR) ? "r" : "-");
    printf((mode & S_IWUSR) ? "w" : "-");
    printf((mode & S_IXUSR) ? "x" : "-");
    printf((mode & S_IRGRP) ? "r" : "-");
    printf((mode & S_IWGRP) ? "w" : "-");
    printf((mode & S_IXGRP) ? "x" : "-");
    printf((mode & S_IROTH) ? "r" : "-");
    printf((mode & S_IWOTH) ? "w" : "-");
    printf((mode & S_IXOTH) ? "x" : "-");
}