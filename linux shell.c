// creating shell that print the computer name and the folder location
// and activate an coammnd that the user insert
// this program implement pipe, redirection and normal command
// cd is not supperted
// adding something to test for
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <string.h>
#include <assert.h>
#include <pwd.h>
#include <fcntl.h>
#include <sys/stat.h>
#define SIZE 510
#define PATH_MAX 4096
#define END_PROGRAM -1
#define NORMAL_COMMAND 0
#define PIPE_COMMAND 1
#define REDIRECTION_BIG 2
#define PIPE_BIG_COMMAND 3
#define REDIRECTION_BIGBIG 4
#define PIPE_BIGBIG_COMMAND 5
#define REDIRECTION_TWOBIG 6
#define PIPE_TWOBIG_COMMAND 7
#define REDIRECTION_SMALL 8
#define PIPE_SMALL_COMMAND 9

void address();
int launchProgram(char *str);
void launchPipe(char *str, int flagCommand);
void launchRedirection(char *str, int flagCommand);
void launchPipeWithRedirection(char *str, int flagCommand);
char **splitLine(char *str, char *remove);
int checkProgram(char **arr);
int openFD(char *str, int flagCommand);
int findCommandType(char *str);
int findPipe(char *str);
char *createFile(char *str);
void freeArr(char **arr);

int main()
{
	int countCommand = 0, commandLength = 0;
	char input[SIZE];

	while (1)
	{
		address();
		fgets(input, SIZE, stdin); // get the input from the user
		if (input[0] != '\n')
		{
			countCommand++;
			commandLength += strlen(input);
			input[strlen(input) - 1] = '\0';
			int x = launchProgram(input);
			if (x == END_PROGRAM)
			{
				break;
			}
		}
	}

	// calculate the num of the order and the average of lenght of commands
	float average = ((float)commandLength) / ((float)countCommand);
	printf("Num of commands: %d\nTotal length of all commands: %d\nAverage length of all commands: %.3f\nSee you Next time!\n", countCommand, commandLength, average);

	return 0;
}

// this method print the address of the computer and the folder
void address()
{
	struct passwd *p;
	__uid_t uid = getuid();
	if ((p = getpwuid(uid)) == NULL)
	{
		perror("error");
	}
	else
	{
		printf("%s@", p->pw_name);
	}
	char cwd[PATH_MAX];
	if (getcwd(cwd, sizeof(cwd)) == NULL)
	{
		perror("error");
	}
	else
	{
		printf("%s>", cwd);
	}
}

// this method is run all the coammnd
int launchProgram(char *str)
{
	char *strCopy = (char *)malloc(sizeof(char) * (strlen(str) + 1));
	assert(strCopy != NULL);
	strncpy(strCopy, str, strlen(str)); // copy of str
	strCopy[strlen(str)] = '\0';

	int isPipeExist = findPipe(str); // check if pipe exist
	int flagCommand = findCommandType(str);
	if (isPipeExist == NORMAL_COMMAND) // check for pipe
	{
		if (flagCommand == NORMAL_COMMAND)
		{
			char **arr = splitLine(strCopy, " "); // crate arr of words
			int check = checkProgram(arr);		  // check for done cd or none input
			if (check == END_PROGRAM)
			{
				freeArr(arr); // free all the memory allocation
				free(strCopy);
				return END_PROGRAM;
			}
			__pid_t pid = fork(); // create a new process (son)
			if (pid < 0)
			{
				perror("ERROR");
				exit(1);
			}
			if (pid == 0)
			{
				int check = execvp(arr[0], arr); // activate the order of the shell
				freeArr(arr);					 // free all the memory allocation
				free(strCopy);
				if (check == -1)
				{
					exit(1);
				}
				exit(0);
			}
			wait(NULL);
			freeArr(arr); // free all the memory allocation
		}
		else if (flagCommand == REDIRECTION_BIG || flagCommand == REDIRECTION_BIGBIG || flagCommand == REDIRECTION_TWOBIG || flagCommand == REDIRECTION_SMALL)
		{
			launchRedirection(strCopy, flagCommand); // run redirection command
		}
	}
	else
	{
		if (flagCommand == PIPE_COMMAND)
		{
			launchPipe(strCopy, flagCommand); // run pipe coammnd
		}
		else
		{
			launchPipeWithRedirection(strCopy, flagCommand); // run pipe with redirection command
		}
	}
	free(strCopy); // free all the memory allocation
	return 0;
}

// this method activate when the user use pipe
void launchPipe(char *str, int flagCommand)
{
	char *strCopy = (char *)malloc(sizeof(char) * (strlen(str) + 1));
	assert(strCopy != NULL);
	strncpy(strCopy, str, strlen(str)); // copy of str
	strCopy[strlen(str)] = '\0';

	char **arr = splitLine(strCopy, "|");

	char **arrPart1 = splitLine(arr[0], " ");
	char **arrPart2 = splitLine(arr[1], " ");

	int pipe_fd[2]; // create pipe
	if (pipe(pipe_fd) == -1)
	{
		perror("pipe failed");
		exit(1);
	}
	__pid_t leftpid = fork(); // create first son
	if (leftpid < 0)
	{
		perror("ERROR");
		exit(1);
	}
	if (leftpid == 0)
	{
		dup2(pipe_fd[1], STDOUT_FILENO); //replacing stdout with pipe write
		close(pipe_fd[0]);				 //closing pipe read
		close(pipe_fd[1]);
		int check = execvp(arrPart1[0], arrPart1);
		if (check == -1)
		{
			exit(1);
		}
		exit(0);
	}
	__pid_t rightpid = fork(); // create second son
	if (rightpid < 0)
	{
		perror("ERROR");
		exit(1);
	}
	if (rightpid == 0) //second fork
	{
		dup2(pipe_fd[0], STDIN_FILENO); //replacing stdin with pipe read
		close(pipe_fd[1]);				//closing pipe write
		close(pipe_fd[0]);
		int check = execvp(arrPart2[0], arrPart2);
		if (check == -1)
		{
			exit(1);
		}
		exit(0);
	}
	close(pipe_fd[0]);
	close(pipe_fd[1]);
	wait(NULL);
	wait(NULL);
	// free all the memory allocation
	free(strCopy);
	freeArr(arr);
	freeArr(arrPart1);
	freeArr(arrPart2);
}

// this method activate when the user use redirection
void launchRedirection(char *str, int flagCommand)
{
	char *strCopy = (char *)malloc(sizeof(char) * (strlen(str) + 1));
	assert(strCopy != NULL);
	strncpy(strCopy, str, strlen(str)); // copy of str
	strCopy[strlen(str)] = '\0';

	char **arr;
	// split the second part to 2 part before redirection and after redirection
	if (flagCommand == REDIRECTION_BIG || flagCommand == REDIRECTION_BIGBIG || flagCommand == REDIRECTION_TWOBIG)
	{
		arr = splitLine(strCopy, ">");
		if (flagCommand == REDIRECTION_TWOBIG)
		{
			arr[0][strlen(arr[0]) - 1] = '\0';
		}
	}
	if (flagCommand == REDIRECTION_SMALL)
	{
		arr = splitLine(strCopy, "<");
	}

	char **leftArr = splitLine(arr[0], " ");

	int fd;
	__pid_t pid = fork(); // create son process
	if (pid < 0)
	{
		perror("ERROR");
		exit(1);
	}
	if (pid == 0)
	{
		char *fileName = createFile(arr[1]); // file name of the folder
		fd = openFD(fileName, flagCommand);  // open file director
		int check = execvp(leftArr[0], leftArr);
		close(fd);
		free(fileName);
		if (check == -1)
		{
			exit(1);
		}
		exit(0);
	}
	wait(NULL);
	// free all the memory allocation
	free(strCopy);
	freeArr(arr);
	freeArr(leftArr);
}

// this method activate when the user use pipe and redirection together
void launchPipeWithRedirection(char *str, int flagCommand)
{
	char *strCopy = (char *)malloc(sizeof(char) * (strlen(str) + 1));
	assert(strCopy != NULL);
	strncpy(strCopy, str, strlen(str)); // copy of str
	strCopy[strlen(str)] = '\0';

	char **arr = splitLine(strCopy, "|");	// split to 2 part before pipe and after pipe
	char **leftArr = splitLine(arr[0], " "); // before pipe

	char **tempArr;
	// split the second part to 2 part before redirection and after redirection
	if (flagCommand == PIPE_BIG_COMMAND || flagCommand == PIPE_BIGBIG_COMMAND || flagCommand == PIPE_TWOBIG_COMMAND)
	{
		tempArr = splitLine(arr[1], ">");
		if (flagCommand == REDIRECTION_TWOBIG)
		{
			tempArr[0][strlen(arr[0]) - 1] = '\0';
		}
	}
	if (flagCommand == PIPE_SMALL_COMMAND)
	{
		tempArr = splitLine(arr[1], "<");
	}
	char **rightArr = splitLine(tempArr[0], " ");

	int pipe_fd[2]; // create the pipe
	if (pipe(pipe_fd) == -1)
	{
		perror("pipe failed");
		exit(1);
	}
	__pid_t leftpid = fork(); // create first son
	if (leftpid < 0)
	{
		perror("ERROR");
		exit(1);
	}
	if (leftpid == 0)
	{
		close(STDOUT_FILENO);
		dup2(pipe_fd[1], STDOUT_FILENO); // replacing stdout with pipe write
		close(pipe_fd[0]);				 // closing pipe read
		close(pipe_fd[1]);

		int check = execvp(leftArr[0], leftArr);
		if (check == -1)
		{
			exit(1);
		}
		exit(0);
	}
	__pid_t rightpid = fork(); // create second son
	if (rightpid < 0)
	{
		perror("ERROR");
		exit(1);
	}
	if (rightpid == 0)
	{
		close(STDIN_FILENO);
		close(pipe_fd[1]);
		char *fileName = createFile(tempArr[1]);	// file name of the folder
		int fd = openFD(fileName, flagCommand - 1); // open file directory

		if (flagCommand == PIPE_BIG_COMMAND || flagCommand == PIPE_BIGBIG_COMMAND || flagCommand == PIPE_TWOBIG_COMMAND)
		{
			dup2(pipe_fd[0], STDIN_FILENO);
		}
		else if (flagCommand == PIPE_SMALL_COMMAND)
		{
			dup2(pipe_fd[0], STDOUT_FILENO);
		}
		close(pipe_fd[0]);

		int check = execvp(rightArr[0], rightArr);
		close(fd);
		free(fileName);
		if (check == -1)
		{
			exit(1);
		}
		exit(0);
	}
	close(pipe_fd[0]);
	close(pipe_fd[1]);
	wait(0);
	wait(0);
	// free all the memory allocation
	free(strCopy);
	freeArr(arr);
	freeArr(leftArr);
	freeArr(tempArr);
	freeArr(rightArr);
}

//open the file directory
int openFD(char *str, int flagCommand)
{
	int fd;
	if (flagCommand == REDIRECTION_BIG)
	{
		fd = open(str, O_WRONLY | O_CREAT | O_TRUNC, S_IRWXU); // >
		if (dup2(fd, STDOUT_FILENO) < 0)
		{
			perror("ERROR");
		}
	}
	if (flagCommand == REDIRECTION_BIGBIG)
	{
		fd = open(str, O_WRONLY | O_CREAT | O_APPEND, S_IRWXU); // >>
		if (dup2(fd, STDOUT_FILENO) < 0)
		{
			perror("ERROR");
		}
	}
	if (flagCommand == REDIRECTION_SMALL)
	{
		fd = open(str, O_RDONLY); // <
		if (dup2(fd, STDIN_FILENO) < 0)
		{
			perror("ERROR");
		}
	}
	if (flagCommand == REDIRECTION_TWOBIG)
	{
		fd = open(str, O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH); // 2>
		if (dup2(fd, STDERR_FILENO) < 0)
		{
			perror("ERROR");
		}
	}
	if (fd < 0)
	{
		perror("ERROR");
	}
	return fd;
}

// this method get a string(line) that the user insert and create arr of words from that line
char **splitLine(char *str, char *remove)
{
	char *strCopy = (char *)malloc(sizeof(char) * (strlen(str) + 1));
	assert(strCopy != NULL);
	strncpy(strCopy, str, strlen(str)); // copy of str
	strCopy[strlen(str)] = '\0';

	int count = 0;
	char **arr = (char **)malloc(SIZE * sizeof(char *));
	assert(arr != NULL);
	char *temp = strtok(strCopy, remove); // split when get to one of the char that inside remove
	while (temp != NULL)
	{
		arr[count] = (char *)malloc(sizeof(char) * (strlen(temp) + 1));
		assert(arr[count] != NULL);
		strncpy(arr[count], temp, strlen(temp) + 1);
		count++;
		temp = strtok(NULL, remove);
	}
	arr = (char **)realloc(arr, sizeof(char *) * (count + 1));
	assert(arr != NULL);
	arr[count] = NULL;

	free(strCopy);
	return arr;
}

// this method 3 possible outcome that the user insert
int checkProgram(char **arr)
{
	if (strcmp(arr[0], "done") == 0)
	{
		return END_PROGRAM;
	}
	if (strcmp(arr[0], "cd") == 0)
	{
		printf("Comand not supperted (Yet)\n");
		return 0;
	}
	return 0;
}

// this method check if there is the char of pipe '|'
int findPipe(char *str)
{
	int i;
	for (i = 0; i < strlen(str); i++)
	{
		if (str[i] == '|')
		{
			return PIPE_COMMAND;
		}
	}
	return NORMAL_COMMAND;
}

// this method return the type of the coammnd
int findCommandType(char *str)
{
	int sum = NORMAL_COMMAND;
	for (int i = 0; i < strlen(str); i++)
	{
		if (str[i] == '|')
		{
			sum = PIPE_COMMAND;
		}
		if (str[i] == '>')
		{
			if (str[i + 1] == '>')
			{
				return (sum + REDIRECTION_BIGBIG);
			}
			else
			{
				return (sum + REDIRECTION_BIG);
			}
		}
		if (str[i] == '2')
		{
			if (str[i + 1] == '>')
			{
				return (sum + REDIRECTION_TWOBIG);
			}
		}
		if (str[i] == '<')
		{
			return (sum + REDIRECTION_SMALL);
		}
	}
	return sum;
}

// delete leading space in the file name for redirection
char *createFile(char *str)
{
	if (str[0] != ' ')
	{
		return str;
	}
	char *file = (char *)malloc(sizeof(char) * (strlen(str)));
	assert(file != NULL);
	int i = 0, j = 0, index = 0;
	for (i = 0; i < strlen(str); i++)
	{
		if (str[i] != ' ')
		{
			break;
		}
	}
	for (j = i; j < strlen(str) + 1; j++)
	{
		file[index] = str[j];
		index++;
	}
	file = (char *)realloc(file, sizeof(char) * (index - 1));
	assert(file != NULL);
	file[strlen(str) - 1] = '\0';
	return file;
}

// free all the memory allocation
void freeArr(char **arr, int a)
{
	int i = 0;
	while (arr[i] != NULL)
	{
		free(arr[i]);
		i++;
	}
	free(arr);
}
