/*
 -------------------------------------
 File:    180930850_a04.c
 Project: 180930850_a04
 file description
 -------------------------------------
 Author:  Daniil Paskal
 ID:      180930850
 Email:   pask0850@mylaurier.ca
 Version  2020-06-21
 -------------------------------------

 GitHub ID: DaniilPaskal
 GitHub Repository: https://github.com/DaniilPaskal/CP386-Assignment-4
 */

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <sys/stat.h>
#include <time.h>
#include <semaphore.h>

typedef struct customer // thread representing customer
{
	int id; // numerical id based on order in which customer is read from file
} Customer;
// this structure ended up being unused and should really have been deleted for the sake of cleaner code

// customer and resource counts
int customer_count = 0;
int resource_count = 0;

// array for customer threads
Customer *customers;

// resource arrays
int *available;
int **maximum;
int **allocation;
int **need;

// work and finish arrays
int *work;
int *finish;

// backup arrays for use in requesting resources
int *s_available;
int **s_allocation;
int **s_need;

// array of customers for safe sequence
int *sequence;

// semaphore for locking arrays when threads are running
sem_t lock;

int read_file(char *file_name, Customer **customers);

int request_resource(char *tokenized_input);
int release_resource(char *tokenized_input);

void starting_data();
void current_state();

void backup(int operation);
int check_safety();

void* thread_run(void *t);

int main(int argc, char *argv[]) {

	//exit program if file name is missing
	/*
	 if (argc < 2) {
	 printf("Input file name missing...exiting with error code -1\n");
	 return -1;
	 }
	 */

	// get resource count from number of inputs
	resource_count = argc - 1;

	// populate available array
	available = malloc(sizeof(int) * resource_count);
	for (int i = 0; i < argc - 1; i++) {
		available[i] = atoi(argv[i + 1]);
	}

	// initialize customer array
	Customer *customers = NULL;

	// read file and populate customer and resource arrays
	read_file("sample4_in.txt", &customers);
	starting_data();

	char input[100];

	// array of input tokens for use in RQ and RL
	int token_count;
	char *token = NULL;
	char *tokenized_input = malloc(sizeof(int) * (resource_count + 2));
	int result;

	// ask for user input until End command is entered
	while (strcmp(input, "End") != 0) {
		printf("Enter Command: ");

		// get user input
		fgets(input, 100, stdin);
		input[strlen(input) - 1] = '\0';

		if (strstr(input, "RQ")) {
			// tokenize input and pass it to request_resource
			token = strtok(input, " ");
			token_count = 0;
			while (token) {
				tokenized_input[token_count] = atoi(token);
				token = strtok(NULL, " ");
				token_count++;
			}

			result = request_resource(tokenized_input);
			if (result == 0) {
				printf("Request is satisfied\n");
			} else {
				printf("Request is denied\n");
			}

		} else if (strstr(input, "RL")) {
			// tokenize input and pass it to release_resource
			token = strtok(input, " ");
			token_count = 0;
			while (token) {
				tokenized_input[token_count] = atoi(token);
				token = strtok(NULL, " ");
				token_count++;
			}

			result = release_resource(tokenized_input);
			printf("Resources released\n");

		} else if (strcmp(input, "*") == 0) {
			current_state();

		} else if (strcmp(input, "Run") == 0) {
			if (check_safety() == 0) {

				// run threads
				pthread_t tid;
				sem_init(&lock, 0, 0);

				for (int i = 0; i < customer_count; i++) {
					printf("--> Customer/Thread %d", sequence[i]);

					printf("\nAllocated resources: ");
					for (int j = 0; j < resource_count; j++) {
						printf(" %d", allocation[sequence[i]][j]);
					}

					printf("\nNeeded: ");
					for (int j = 0; j < resource_count; j++) {
						printf(" %d", need[sequence[i]][j]);
					}

					printf("\nAvailable: ");
					for (int j = 0; j < resource_count; j++) {
						printf(" %d", available[j]);
					}
					printf("\n");

					pthread_create(&tid, NULL, thread_run, &sequence[i]);

					printf("New Available: ");
					for (int j = 0; j < resource_count; j++) {
						printf(" %d", available[j]);
					}
					printf("\n");

				}
			} else {
				printf("Safe sequence not found\n");
			}

		} else if (strcmp(input, "End") == 0) {
			break;

		} else {
			printf("Invalid command\n");
			printf("Valid commands are RQ, RL, *, Run, and End\n");
		}
	}

	// free all arrays
	free(available);
	free(maximum);
	free(allocation);
	free(need);
	free(work);
	free(finish);
	free(s_available);
	free(s_allocation);
	free(s_need);
	free(sequence);

	return 0;

}

int read_file(char *file_name, Customer **customers) {
	FILE *in = fopen(file_name, "r");

	// exit program if file cannot be read
	if (!in) {
		printf(
				"Child A: Error in opening input file...exiting with error code -1\n");
		return -1;
	}

	struct stat st;
	fstat(fileno(in), &st);
	char *fileContent = (char*) malloc(((int) st.st_size + 1) * sizeof(char));
	fileContent[0] = '\0';
	while (!feof(in)) {
		char line[100];
		if (fgets(line, 100, in) != NULL) {
			strncat(fileContent, line, strlen(line));
		}
	}
	fclose(in);

	// get customer_count
	char *command = NULL;
	char *fileCopy = (char*) malloc((strlen(fileContent) + 1) * sizeof(char));
	strcpy(fileCopy, fileContent);
	command = strtok(fileCopy, "\r\n");
	while (command != NULL) {
		customer_count++;
		command = strtok(NULL, "\r\n");
	}

	// initialize maximum, allocation, and need arrays
	maximum = malloc(sizeof(int*) * customer_count);
	allocation = malloc(sizeof(int*) * customer_count);
	need = malloc(sizeof(int*) * customer_count);
	for (int i = 0; i < customer_count; i++) {
		maximum[i] = malloc(sizeof(int) * resource_count);
		allocation[i] = malloc(sizeof(int) * resource_count);
		need[i] = malloc(sizeof(int) * resource_count);
	}

	// initialize work and finish arrays
	work = malloc(sizeof(int) * resource_count);
	finish = malloc(sizeof(int) * customer_count);

	// create customer array
	*customers = (Customer*) malloc(sizeof(Customer) * customer_count);

	char *lines[customer_count];
	command = NULL;
	int i = 0;
	command = strtok(fileContent, "\r\n");
	while (command != NULL) {
		lines[i] = malloc(sizeof(command) * sizeof(char));
		strcpy(lines[i], command);
		i++;
		command = strtok(NULL, "\r\n");
	}

	// populate maximum array
	for (int k = 0; k < customer_count; k++) {
		(*customers)[k].id = k;

		char *token = NULL;
		int j = 0;
		token = strtok(lines[k], ",");
		while (token != NULL) {
			if (j > resource_count) {
				printf(
						"Warning: resources in text file exceed resources given in input\n");
			}

			maximum[k][j] = atoi(token);

			j++;
			token = strtok(NULL, ",");
		}
	}

	// initialize all values in allocation and finish to 0 and all values in need to maximum
	for (int i = 0; i < customer_count; i++) {
		finish[i] = 0;
		for (int j = 0; j < resource_count; j++) {
			allocation[i][j] = 0;
			need[i][j] = maximum[i][j];
		}
	}

	// populate work array
	for (int i = 0; i < resource_count; i++) {
		work[i] = available[i];
	}

	// initialize backup arrays
	s_available = malloc(sizeof(int) * resource_count);
	s_allocation = malloc(sizeof(int*) * customer_count);
	s_need = malloc(sizeof(int*) * customer_count);
	for (int i = 0; i < customer_count; i++) {
		s_allocation[i] = malloc(sizeof(int) * resource_count);
		s_need[i] = malloc(sizeof(int) * resource_count);
	}

	// initialize sequence array
	sequence = malloc(sizeof(int) * customer_count);

	return customer_count;
}

// requests resource to customer (returns 0 if successful and -1 if unsuccessful)
int request_resource(char *tokenized_input) {
	int customer = tokenized_input[1];

	if (customer >= customer_count) {
		printf("Invalid customer\n");
		return -1;
	}

	// iterate through resources
	for (int i = 0; i < resource_count; i++) {
		// if request exceeds need, deny
		if (tokenized_input[i + 2] > need[customer][i]) {
			return -1;
		}

		// if request exceeds available, deny
		if (tokenized_input[i + 2] > available[i]) {
			return -1;
		}
	}

	// save copies of available, allocation, and need to restore from if request is unsafe
	backup(1);

	// available -= request
	for (int i = 0; i < resource_count; i++) {
		available[i] -= tokenized_input[i + 2];
	}

	// allocation += request; need -= request
	for (int i = 0; i < resource_count; i++) {
		allocation[customer][i] += tokenized_input[i + 2];
		need[customer][i] -= tokenized_input[i + 2];
	}

	// check safety of request for each resource
	// if request is unsafe, restore and deny
	for (int i = 0; i < resource_count; i++) {
		if (need[customer][i] > available[i]) {
			backup(0);
			return -1;
		}
	}

	return 0;
}

// release resource from customer (returns 0 if successful and -1 if unsuccessful)
int release_resource(char *tokenized_input) {
	int customer = tokenized_input[1];

	if (customer >= customer_count) {
		printf("Invalid customer\n");
		return -1;
	}

	// decrease allocation and increase available by given number for each resource
	for (int i = 0; i < resource_count; i++) {
		// if release request exceeds resources in allocation, increase available to allocation and set allocation to 0
		if (allocation[customer][i] - tokenized_input[i + 2] <= 0) {
			available[i] += allocation[customer][i];
			allocation[customer][i] = 0;
		} else {
			available[i] += tokenized_input[i + 2];
			allocation[customer][i] -= tokenized_input[i + 2];
		}

	}

	return 0;
}

// prints data given at start
void starting_data() {
	printf("Number of Customers: %d\n", customer_count);
	printf("Currently Available resources:");
	for (int i = 0; i < resource_count; i++) {
		printf(" %d", available[i]);
	}
	printf("\nMaximum resources from file:\n");
	for (int i = 0; i < customer_count; i++) {
		for (int j = 0; j < resource_count; j++) {
			printf("%d", maximum[i][j]);
			if (j < resource_count - 1) {
				printf(",");
			}
		}
		printf("\n");
	}
}

// prints current state of arrays when user inputs "*"
void current_state() {
	printf("Available:\n");
	for (int i = 0; i < customer_count; i++) {
		printf("%d", available[i]);
		if (i < resource_count - 1) {
			printf(",");
		}
	}
	printf("\n");
	printf("Maximum:\n");
	for (int i = 0; i < customer_count; i++) {
		for (int j = 0; j < resource_count; j++) {
			printf("%d", maximum[i][j]);
			if (j < resource_count - 1) {
				printf(",");
			}
		}
		printf("\n");
	}
	printf("Allocation:\n");
	for (int i = 0; i < customer_count; i++) {
		for (int j = 0; j < resource_count; j++) {
			printf("%d", allocation[i][j]);
			if (j < resource_count - 1) {
				printf(",");
			}
		}
		printf("\n");
	}
	printf("Need:\n");
	for (int i = 0; i < customer_count; i++) {
		for (int j = 0; j < resource_count; j++) {
			printf("%d", need[i][j]);
			if (j < resource_count - 1) {
				printf(",");
			}
		}
		printf("\n");
	}
}

// saves from main arrays to backup arrays if operation = 1, loads from backup to main if operation = 0
void backup(int operation) {
	if (operation == 1) {
		for (int i = 0; i < customer_count; i++) {
			for (int j = 0; j < resource_count; j++) {
				s_available[j] = available[j];
				s_allocation[i][j] = allocation[i][j];
				s_need[i][j] = need[i][j];
			}
		}
	} else if (operation == 0) {
		for (int i = 0; i < customer_count; i++) {
			for (int j = 0; j < resource_count; j++) {
				available[j] = s_available[j];
				allocation[i][j] = s_allocation[i][j];
				need[i][j] = s_need[i][j];
			}
		}
	}
}

// finds safe sequence
int check_safety() {
	// initialize work and finish
	for (int i = 0; i < customer_count; i++) {
		finish[i] = 0;
		sequence[i] = 0;
	}
	for (int i = 0; i < resource_count; i++) {
		work[i] = available[i];
	}

	int resources_unavailable = 0;
	int customer_present = 0;
	int sequence_counter = 0;

	// iterate through customers
	for (int i = 0; i < customer_count; i++) {
		if (finish[i] == 0) {
			resources_unavailable = 0;
			// check if needs exceed available
			for (int j = 0; j < resource_count; j++) {
				if (need[i][j] > available[j]) {
					resources_unavailable = 1;
					break;
				}

				// if needs do not exceed available, proceed
				if (resources_unavailable == 0) {
					customer_present = 0;
					// check if customer is already in sequence
					for (int k = 0; k < customer_count; k++) {
						if (sequence[k] == i) {
							customer_present = 1;
						}
					}

					// if customer not present, proceed
					if (customer_present == 0) {
						// available += allocation, finish[i] = 1
						for (int j = 0; j < resource_count; j++) {
							available[j] += allocation[i][j];
						}
						finish[i] = 1;

						// add customer to sequence
						sequence[sequence_counter] = i;
						sequence_counter++;
					}
				}
			}
		}
	}
	// if any finish[i] == 0, sequence is unsafe
	for (int i = 0; i < customer_count; i++) {
		if (finish[i] == 0) {
			return -1;
		}
	}

	return 0;
}

void* thread_run(void *t) {
	int *customer = (int*) t;

	// lock semaphore
	sem_wait(&lock);

	printf("Thread has started\n");

	// allocation = need, available -= need
	for (int i = 0; i < resource_count; i++) {
		allocation[*customer][i] = need[*customer][i];
		available[i] -= need[*customer][i];
	}

	printf("Thread has finished\n");
	printf("Releasing resources\n");

	// release allocated back to available and set need to 0
	for (int i = 0; i < resource_count; i++) {
		available[i] += allocation[*customer][i];
		need[*customer][i] = 0;
	}

	// unlock semaphore
	sem_post(&lock);

	pthread_exit(0);
}
