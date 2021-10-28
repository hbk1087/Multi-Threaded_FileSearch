#include <pthread.h>
#include <stdio.h>
#include <sys/stat.h> 
#include <unistd.h>
#include <stdlib.h>
#include <dirent.h> 
#include <string.h>
#include <fcntl.h> 
#include <ctype.h>
#include <string.h>

//Node structure that will be used to store file names and words in files.
struct node
{
	struct node * next;
	char * data;
	int position;
	char wordsAfter[50];
	int visited;
	struct node* words;
};

//Queue structure that will be used to store file and directory locations used for thread functions.
struct queue
{
    struct node* head;  
    pthread_mutex_t lock;
};

//Structure that holds all the data that is needed for thread functions.
struct threadData
{
	struct queue* fileQueue;
	struct queue* directoryQueue;
	struct node* fileNameMatches;
	struct node* keyWordMatches;
	struct node* phraseMatches;
	struct node* inputWords;
	struct node* fileWords;
	int activeDThreads;
	int activeFThreads;
	pthread_t *dirPointer;
	pthread_t *filePointer;
	int fileIDCount;
	int dirIDCount; 
	char* input;
	pthread_mutex_t lock; 
};

int isDirectory(char * name);
int isFile(char * name);
struct node* allocateNode(char* data); 
struct node* insert(struct node* head, char* data, int position);
struct node* insertNode(struct node* head, struct node* toAdd, int position);
int getLength(struct node* head);
void printReverse(struct node* head, int count);
void printOnlyFile(char* data);
void onlyPrintReg(char* data);
void printKeyWords(struct node* head);
void printLocation(char* data);
char* onlyFile(char* data);
void freeNode(struct node* ptr);
char* toLower(char* c);
struct queue* allocateQueue(); 
struct threadData* allocateThreadData();
struct queue* enqueue(struct queue* queue, char* data);
struct queue* dequeue(struct queue* queue); 
void freeQueue(struct queue* queue);
void freeWords(struct node* ptr);
void freeAll(struct threadData* td);
void *directorySearch(void *A);
void * fileSearch (void * A);
void splitWords(struct threadData* td, char* bytes);
void compare(struct threadData* td, struct node* ptr);
struct node* sort(struct node* keyWordMatches);
int results;
struct node* addNode(struct node* target, struct node* toAdd);
struct node* copyNode(struct node* target, struct node* toCopy);

//Main function that scans in input and outputs results.
int main(int argc, char** argv)
{
	int dirThreads = 1;
	printf("\nPlease enter the file path to the directory you would like to search.\n");
	char directoryName[250];
	scanf("%s", directoryName); 
	struct threadData* td = allocateThreadData(); 
	if (isDirectory(directoryName) == 1)
	{
		td->directoryQueue = enqueue(td->directoryQueue, directoryName);
	}
	else 
	{
		free(td->directoryQueue);
		freeQueue(td->fileQueue);
		free(td);
		return EXIT_FAILURE; 
	}
	printf("\nSearch by keywords, partial/full file names, or phrases in a specific order.\n");
	char* input = malloc(1000 * sizeof(char));
	char clear[250];
	scanf("%c",clear);
	scanf("%[^\n]",input);
	td->input = malloc(1000 * sizeof(char));
	strcpy(td->input, input);
	splitWords(td, input);
	td->dirPointer = (pthread_t*) malloc(dirThreads * sizeof(pthread_t));
	td->filePointer = (pthread_t*) malloc(1 * sizeof(pthread_t));
	printf("\nHow many results would you like to be displayed?\n");
	scanf("%d", &results);
	int temp = results;
	td->activeDThreads = 1;
	directorySearch(td);
	while(td->activeFThreads != 0 || td->fileQueue->head != 0 || td->activeDThreads != 0 || td->directoryQueue->head != 0)
	{
		continue;
	}
	if (td->fileNameMatches == NULL)
	{
		printf("\nNo file name matches.\n");
	}
	else
	{
		int length = getLength(td->fileNameMatches);
		if (results > length)
		{
			results = length;
		}
		printf("\nFile name matches:\n");
		printReverse(td->fileNameMatches, length);
	}
	results = temp;
	if (td->phraseMatches == NULL)
	{
		printf("\nNo matching phrases in text.\n");
	}
	else
	{
		int length = getLength(td->phraseMatches);
		if (results > length)
		{
			results = length;
		}
		printf("\nMatching phrases in text:\n");
		printReverse(td->phraseMatches, length);
	}
	results = temp;
	if(td->keyWordMatches == 0)
	{
		printf("\nNo matching words in text.\n");
	}
	else
	{
		td->keyWordMatches = sort(td->keyWordMatches);
		int length = getLength(td->keyWordMatches);
		if (results > length)
		{
			results = length;
		}
		printf("\nMatching words in text:\n");
		printReverse(td->keyWordMatches, length);
	}
	free(input);
	free(td->input);
	freeAll(td);
	return EXIT_SUCCESS;
}

//Checks if string refers to a file location.
int isFile(char * name)
{
	struct stat statbuf;
	if (stat(name, &statbuf) != 0)
	{
		printf("Could not open file: %s\n", name);
    	return 0;
	}
	return S_ISREG(statbuf.st_mode);
}

//Checks if string refers to a directory location.
int isDirectory(char * name)
{
	struct stat statbuf;
	if (stat(name, &statbuf) != 0)
	{
		printf("Could not open directory: %s\n", name);
    	return 0;
	}
	return S_ISDIR(statbuf.st_mode);
}

//Method that allocates memory for node and data.
struct node* allocateNode(char* data)
{
	struct node* temp = malloc(sizeof(struct node));
	int num = strlen(data) + 1;
	temp->data = malloc(num * sizeof(char));
	memset(temp->data, '\0', num);
	strcpy(temp->data, data);
	temp->next = 0;
	memset(temp->wordsAfter, '\0', 50);
	temp->visited = 0;
	temp->words = 0;
	return temp;
}

//Method that creates a new node and inserts it into the list.
struct node* insert(struct node* head, char* data, int position)
{
	struct node* ptr = head;
	struct node* prev = 0;
	if(head == 0)
	{
		head = allocateNode(data);
		head->position = position;
		return head;		
	}	
	while(ptr != 0)
	{
		if (ptr->position == position || ptr->position > position)
		{
			struct node* temp = allocateNode(data);
			temp->position = position;
			if (prev == 0)
			{
				temp->next = ptr;
				return temp;
			}
			prev->next = temp;
			temp->next = ptr;
			return head;
		}
		else if (ptr->next == 0 && ptr->position < position)
		{	
			struct node* temp = allocateNode(data);
			temp->position = position;
			ptr->next = temp;
			return head;
		}
		prev = ptr;
		ptr = ptr->next;
	}
	return head;
}

//Method that inserts an already existing node into a list.
struct node* insertNode(struct node* head, struct node* toAdd, int position)
{
	struct node* ptr = head;
	struct node* prev = 0;
	if(head == 0)
	{
		head = copyNode(head, toAdd);
		return head;		
	}	
	while(ptr != 0)
	{
		if (ptr->position == position || ptr->position > position)
		{
			struct node* temp = 0;
			temp = copyNode(temp, toAdd);
			temp->position = position;
			if (prev == 0)
			{
				temp->next = ptr;
				return temp;
			}
			prev->next = temp;
			temp->next = ptr;
			return head;
		}
		else if (ptr->next == 0 && ptr->position < position)
		{	
			struct node* temp = 0;
			temp = copyNode(temp, toAdd);
			temp->position = position;
			ptr->next = temp;
			return head;
		}
		prev = ptr;
		ptr = ptr->next;
	}
	return head;
}

//Method that copies a nodes values into a target node.
struct node* copyNode(struct node* target, struct node* toCopy)
{
	struct node* ptr = toCopy->words;
	target = allocateNode(toCopy->data);
	{
		while(ptr != 0)
		{
			if(target->words == 0)
			{
				target->words = allocateNode(ptr->data);
				target->words->position = ptr->position;
			}
			else
			{
				struct node* tprev = 0;
				struct node* tptr = target->words;
				while(tptr != 0)
				{
					tprev = tptr;
					tptr = tptr->next;
				}
				if(tprev == 0)
				{
					tptr->next = allocateNode(ptr->data);
					tptr->next->position = ptr->position;
				}
				else
				{
					struct node* temp = allocateNode(ptr->data);
					temp->position = ptr->position;
					tprev->next = temp;
					temp->next = tptr;
				}
				
			}
		ptr = ptr->next;
		}
	}
	return target;
}

//Method that prints the linked list of files in reverse order.
void printReverse(struct node* head, int count)
{
    if (head == NULL)
	{
       return;
	}	
    printReverse(head->next, --count);
	if (results == 0)
	{
       return;
	}	
	printf("\n%d: ", count);
	printOnlyFile(head->data);
	if(head->wordsAfter[0] != '\0')
	{
	
		printf("\n\"");
		onlyPrintReg(head->wordsAfter);
		printf("...\"");
	}
	if (head->words != 0)
	{
		printf("\n");
		printKeyWords(head->words);
	}
    printf("\nLocated in: ");
	printLocation(head->data);
	printf("\n");
	results--;
}

//Prints the matching key words in files.
void printKeyWords(struct node* head)
{
		if(head == NULL)
		{
			return;
		}
		printKeyWords(head->next);
		if(head->next == NULL)
		{
		printf("%s: %d ", head->data, head->position);
		}
		else
		{
			printf("| %s: %d ", head->data, head->position);
		}
}

//Converts a new string to lower case.
char* toLower(char* c)
{
	int size = strlen(c);
	char* temp = malloc((size +1) * sizeof(char));
	memset(temp, '\0', size);
	strcpy(temp, c);
	for(int x = 0; x < size; x++)
	{
		temp[x] = tolower(temp[x]);
	}
	return temp;
}

//Gets the length of a node.
int getLength(struct node* head)
{
	int count = 1;
	while(head != NULL)
		{
			head = head->next;
			count++;
		}
		return count;
}

//Prints out only normal characters from a string.
void onlyPrintReg(char* data)
{
	for (int x = 0; x < strlen(data); x++)
	{
		if (data[x] == '\n')
		{
			printf(" ");
		}
		else if(isprint(data[x]))
		{
			printf("%c", data[x]);
		}
	}
}

//Takes in a file pathway and only prints out the file name.
void printOnlyFile(char* data)
{
	if (strstr(data, "/") == 0)
	{
		printf("%s", data);
		return;
	}
	int count = 0;
	for(int x = 0; x < strlen(data); x++)
	{
		if (data[x] == '/')
		{
			count = x;
		}
	}
	count++;
	for (; count < strlen(data); count++)
	{
		printf("%c", data[count]);
	}
}

//Takes in file pathwway and returns only file name.
char* onlyFile(char* data)
{
	if (strstr(data, "/") == 0)
	{
		return data;
	}
	int count = 0;
	int size = strlen(data);
	for(int x = 0; x < size; x++)
	{
		if (data[x] == '/')
		{
			count = x;
		}
	}
	count++;
	int toDel = 0;
	for (int x = 0; count < size; count++, x++)
	{
		data[x] =  data[count];
		toDel = x;
	}
	for(int x = size - 1; x >  toDel; x--)
	{
		data[x] = '\0';
	}
	return data;
}

//Takes in file pathway and prints out file location.
void printLocation(char* data)
{
	int index = 0;
	for(int x = strlen(data)-1; x > 0; x--)
	{
		if (data[x] == '/')
		{
			index = x;
			break;
		}
	}
	for(int x = 0; x < index; x++)
	{
		printf("%c", data[x]);
	}
}

//Method to allocate space for queue and data.
struct queue* allocateQueue()
{
	struct queue* temp = malloc(sizeof(struct queue));
	temp->head = 0;
    pthread_mutex_init(&temp->lock, NULL);
	return temp;
}

//Method to add to a queue.
struct queue* enqueue(struct queue* queue, char* data)
{
	pthread_mutex_lock(&queue->lock);
	if(queue->head == 0)
	{	
		struct node* temp = allocateNode(data);
		queue->head = temp;
		pthread_mutex_unlock(&queue->lock);
		return queue; 
	}
	struct node* ptr = queue->head;
	struct node* prev = 0;
	while (ptr != 0)
	{
		prev = ptr;
		ptr = ptr->next;
	}
	struct node* temp = allocateNode(data);
	prev->next = temp;
	pthread_mutex_unlock(&queue->lock);
	return queue; 
}

//Method to delete from the queue.
struct queue* dequeue(struct queue* queue)
{
	pthread_mutex_lock(&queue->lock);
	struct node* temp = queue->head;
	queue->head = queue->head->next;
	free(temp->data); 
	free(temp);
	pthread_mutex_unlock(&queue->lock);
	return queue; 
}

//Method to allocate space for thread data struct.
struct threadData* allocateThreadData()
{
	struct threadData* temp = malloc(sizeof(struct threadData));
	temp->fileQueue = allocateQueue();
	temp->directoryQueue = allocateQueue();
	temp->fileNameMatches = 0;
	temp->phraseMatches = 0;
	temp->keyWordMatches = 0;
	temp->inputWords = 0;
	temp->fileWords = 0;
	temp->fileIDCount = 0;
	temp->dirIDCount = 1; 
	temp->activeFThreads = 0;
	pthread_mutex_init(&temp->lock, NULL);
	return temp;
}

//Directory thread that iterates through directories and enqueues new directories into the directory Queue and enqueues encountered files into the file Queue. Creates new threads each time directory or file is encountered.
void *directorySearch(void *A)
{
	struct threadData *param = A;
	if ((param->directoryQueue->head != 0) && param->directoryQueue->head->visited == 0)
	{
		param->directoryQueue->head->visited++;
		int dirSize= strlen(param->directoryQueue->head->data) + 1;
		char* nameOfDirectory = malloc(dirSize* sizeof(char));
		memset(nameOfDirectory, '\0', dirSize);
		strcpy(nameOfDirectory, param->directoryQueue->head->data);
		param->directoryQueue = dequeue(param->directoryQueue);
		DIR * directory = opendir(nameOfDirectory);
		struct dirent *iterate;
		while ((iterate = readdir(directory))) 
		{
			dirSize = strlen(nameOfDirectory) + strlen(iterate->d_name) + 2;
			char* nameOfFile = malloc(dirSize * sizeof(char));
			memset(nameOfFile, '\0', dirSize);
			nameOfFile = strcpy(nameOfFile, nameOfDirectory);
			nameOfFile = strcat(nameOfFile, "/");
			nameOfFile = strcat(nameOfFile, iterate->d_name);
			if(isFile(nameOfFile) == 1)
			{
				param->fileQueue = enqueue(param->fileQueue, nameOfFile);
				int idSize = sizeof(param->filePointer)  / sizeof(*param->filePointer);
				if (param->fileIDCount >= idSize)
				{
					pthread_mutex_lock(&param->lock);
					pthread_t * nameOfFile = (pthread_t*) realloc(param->filePointer, 2 *  param->fileIDCount * sizeof(pthread_t));
					param->filePointer = nameOfFile;
					pthread_mutex_unlock(&param->lock);
				}
				pthread_create(&param->filePointer[param->fileIDCount++], NULL, fileSearch, param);
			}
			else if (strstr(nameOfFile, "/.") == 0 && isDirectory(nameOfFile) == 1)
			{
					param->directoryQueue = enqueue(param->directoryQueue, nameOfFile);
					int idSize = sizeof(param->dirPointer)  / sizeof(*param->dirPointer);
					if (param->dirIDCount >= idSize)
					{
						pthread_mutex_lock(&param->lock);
						pthread_t * nameOfFile = (pthread_t*) realloc(param->dirPointer, 2 *  param->dirIDCount * sizeof(pthread_t));
						param->dirPointer = nameOfFile;
						pthread_mutex_unlock(&param->lock);
					}
					param->activeDThreads++;
					pthread_create(&param->dirPointer[param->dirIDCount++], NULL, directorySearch, param);
			}
		free(nameOfFile);
		}
		free(nameOfDirectory);
		closedir(directory);
	}
	param->activeDThreads--;
	return NULL;
} 

//File thread that scans the file and looks for matching kerywords, filenames, and phrases. This method will add data into appropitate linked list in thread data struct.
void * fileSearch (void * A)
{
	struct threadData *param = A;
	param->activeFThreads++;
	if(	param->fileQueue->head == 0 || param->fileQueue->head->visited != 0)
	{
		param->activeFThreads--;
		return NULL;
	}
	param->fileQueue->head->visited++;
	char keyword[200];
	memset(keyword, '\0', 200);
	int wordLength = 0;
	int num = strlen(param->fileQueue->head->data) + 1;
	char* fileName = malloc(num * sizeof(char));
	memset(fileName, '\0', num);
	strcat(fileName, param->fileQueue->head->data);
	param->fileQueue = dequeue(param->fileQueue);
	char* fileIngoreCase = toLower(fileName);
	char* inputIgnoreCase = toLower(param->input);
	if(strstr(onlyFile(fileIngoreCase),inputIgnoreCase) != 0)
	{
		param->fileNameMatches = insert(param->fileNameMatches, fileName, 0);
	}
	free(fileIngoreCase);
	char bytes[500];
	int file = open(fileName, O_RDONLY);
	int bytesRead;
	int found = 0;
	struct node* ptr = 0;
	int memSize = 500;
	char* allBytes = malloc(memSize * sizeof(char));
	memset(allBytes, '\0', 500);
	while ((bytesRead = read(file, bytes, 499)) > 0)
	{
		memSize+= 499;
		allBytes = realloc(allBytes, memSize * sizeof(char));
		strcat(allBytes, bytes);
		memset(bytes, '\0', 500);
	}
	char* fullString = toLower(allBytes);
	char* c = strstr(fullString, inputIgnoreCase);
	int size = strlen(allBytes);
	if(c != 0  && found == 0)
	{
		int position = c - fullString;
		param->phraseMatches = insert(param->phraseMatches, fileName, 0);
		found = 1;
		int charToPrint = 49;
		if (position > size - 49)
		{
			charToPrint = size  - position - 1;
		}
		int count = 0;
		for(int x = position; x < charToPrint + position; x++)
		{
		param->phraseMatches->wordsAfter[count] = allBytes[x];
		count++;
		}
	}
	free(allBytes);
	int bol = 0;
	for(int x = 0; x < size; x++)
	{
		if (x == size - 1 || (isspace(fullString[x]) != 0 && wordLength > 0) || fullString[x] == '/')
		{
			if (bol == 0)
			{
				ptr = insert(ptr, fileName, 0);
				bol = 1;
			}
			ptr->words = insert(ptr->words, keyword, 0); 
			memset(keyword, '\0', 200);
			wordLength = 0;
		}
		else if (isspace(fullString[x]) == 0)
		{
			if(fullString[x] == '-' || ispunct(fullString[x]) == 0)
			{
				keyword[wordLength] = fullString[x];
				wordLength++;
			}
		}
	}
	free(fullString);
	if(ptr != 0)
	{
		compare(param, ptr);
	}
	free(inputIgnoreCase);
	free(fileName);
	close(file);
	param->activeFThreads--;
	return NULL;
}

//Method that splits up the terminal input as distinct keywords and adds them to the appropitate linked list.
void splitWords(struct threadData* td, char* bytes)
{
	char* word = malloc(1000 * sizeof(char));
	memset(word, '\0', 1000);
	int size = strlen(bytes);
	int length = 0;
	for(int x = 0; x <= size; x++)
	{
		if (((isspace(bytes[x]) != 0) && length > 0) || x == size)
		{
			td->inputWords = insert(td->inputWords, word, 0); 
			memset(word, '\0', 100);
			length = 0;
		}
		else if ((ispunct(bytes[x]) == 0 || bytes[x] == '-') && isspace(bytes[x]) == 0)
		{
			word[length] = tolower(bytes[x]);
			length++;
		}
	}
	free(word);
}

//Method that adds a node to the back of an already existing linked list.
struct node* addNode(struct node* target, struct node* toAdd)
{
	struct node* ptr = target;
	if(ptr  == 0)
	{
		return toAdd;
	}
	if(ptr->next == 0)
	{
		ptr->next = toAdd;
		return ptr;
	}
	while(ptr->next != 0)
	{
		ptr = ptr->next;
	}
	ptr->next = toAdd;
	return target;
}

//Method to sort the files containing key word matches. It will take into account the number of each keyword encountered as well as the number of distinct keywords encountered.
struct node* sort(struct node* keyWordMatches)
{
	struct node* file = keyWordMatches;
	struct node* word =  keyWordMatches->words;
	struct node* sorted = 0;
	int numWords = 0;
	int numMatches= 0;
	int position = 0;
	while(file != 0)
	{ 
		numMatches = 0;
		word = file->words;
		numWords = 0;
		position = 0;
		while(word != 0)
		{
			numMatches++;
			numWords+= word->position;
			word = word->next;
		}
		position = numMatches * 1000 + numWords;
		sorted = insertNode(sorted, file, position);
		file = file->next;
	}
	freeNode(keyWordMatches);
	return sorted;
}

//Method to compare the keywords inputed in the terminal to the keywords of a file. If there is a match it will add the file and all of the matching key words to the appropitate linked list.
void compare(struct threadData* td, struct node* ptr)
{
	struct node* iptr = td->inputWords;
	struct node* wptr = ptr->words;
	struct node* matches = 0;
	int bol = 0;
	while(iptr != 0)
	{	
		wptr = ptr->words;
		int count = 0;
		while(wptr != 0)
		{
			if (strcmp(iptr->data, wptr->data) == 0)
			{
				if(wptr->visited == 1)
				{
					break;
				}
				count++;
				wptr->visited = 1;
			}
			wptr = wptr->next;
		}
		if(count > 0)
		{
			if(bol == 0)
			{
				matches = insert(matches, ptr->data, 0);
				bol =1;
			}
			matches->words = insert(matches->words, iptr->data, count);
		}
		iptr = iptr->next;
	}
	if(bol == 1)
	{
		td->keyWordMatches = addNode(td->keyWordMatches, matches);
	}
	freeNode(ptr);
}

//method to free all the nodes containing key words.
void freeWords(struct node* ptr)
{
	struct node* prev = 0;
	while(ptr->words != 0)
	{
		prev = ptr->words;
		ptr->words = ptr->words->next;
		free(prev->data);
		free(prev);
	}
}

//Method to fre an entire linked list.
void freeNode(struct node* ptr)
{
	struct node* prev = 0;
	if (ptr == 0)
	{
		return;
	}

	while(ptr != 0)
	{
		freeWords(ptr);
		prev = ptr;
		ptr = ptr->next;
		free(prev->data);
		free(prev);
	}
}

//Method to free a queue and all of its elements.
void freeQueue(struct queue* queue)
{
	freeNode(queue->head);
	free(queue);
}

//Method to free all of the data in the main thread struct.
void freeAll(struct threadData* td)
{
	freeNode(td->fileNameMatches);
	freeNode(td->phraseMatches);
	freeNode(td->keyWordMatches);
	freeNode(td->inputWords);
	freeNode(td->fileWords);
	freeQueue(td->directoryQueue);
	freeQueue(td->fileQueue);
	free(td->dirPointer);
	free(td->filePointer);
	free(td);
}
