/*
 -------------------------------------
 File:    180930850_a01.c
 Project: 180930850_a01
 file description
 -------------------------------------
 Author:  Daniil Paskal
 ID:      180930850
 Email:   pask0850@mylaurier.ca
 Version  2020-06-21
 -------------------------------------
 */

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <sys/stat.h>
#include <time.h>
#include <semaphore.h>

// initialize customer and resource counts
int customer_count = 0;
int resource_count = 0;

typedef struct customer // thread representing customer
{
	int id; // numerical id based on order in which customer is read from file

	int *demanded; // array of resources, where each number represents max demand of resource
	int *allocated; // array of resources, where each number represents amount of resource already allocated
	int *needed; // array of resources, where each number represents amount of resources still needed

	pthread_t handle;
} Customer;

typedef struct resource {
	int id; // numerical id based on order in which resources are written in sample file
	int available;	// number representing available amount of resource

	int *demanded; // array of customers, where each number represents max resource demand of customer
	int *allocated;	// array of customers, where each number represents amount of resource currently allocated to customer
	int *needed; // array of customers, where each number represents amount needed by customer

} Resource;

int main(int argc, char *argv[]) {

	// exit program if file name is missing
	if (argc < 2) {
		printf("Input file name missing...exiting with error code -1\n");
		return -1;
	}

	// declare arrays for customers and resources
	Customer *customers;
	Resource *resources;

	// read file and populate customer and resource arrays
	read_file(argv[1], &customers, &resources);

}

void read_file(char *file_name, Customer **customers, Resource **resources) {
	FILE *in = fopen(file_name, "r");

	// exit program if file cannot be read
	if (!in) {
		printf(
				"Child A: Error in opening input file...exiting with error code -1\n");
		return;
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

	char *command = NULL;
	int threadCount = 0;
	char *fileCopy = (char*) malloc((strlen(fileContent) + 1) * sizeof(char));
	strcpy(fileCopy, fileContent);
	command = strtok(fileCopy, "\r\n");
	while (command != NULL) {
		customer_count++;
		command = strtok(NULL, "\r\n");
	}

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

	// get resource count from 1st line
	char *token = strtok(lines[0], ",");
	while (token != NULL) {
		resource_count++;
		token = strtok(NULL, ";");
	}

	// create resource array
	*resources = (Resource*) malloc(sizeof(Resource) * resource_count);

	// read 1st line again, populating resource array with resources
	char *token = strtok(lines[0], ",");
	int k = 0;
	while (token != NULL) {
		(*resources)[k].id = k;
		create_resource(*resources[k]);
		token = strtok(NULL, ";");
	}

	for (int k = 0; k < customer_count; k++) {
		(*customers)[k].id = k;

		token = NULL;
		int j = 0;
		token = strtok(lines[k], ",");
		while (token != NULL) {
			(*customers)[k].demanded[j] = atoi(token);
			(*customers)[k].allocated[j] = 0;
			(*customers)[k].needed[j] = atoi(token);

			(*resources)[j].demanded[k] = atoi(token);
			(*resources)[j].allocated[k] = 0;
			(*resources)[j].needed[k] = atoi(token);

			j++;
			token = strtok(NULL, ";");
		}
	}
	return;
}

void create_customer(Customer *customer) {
	*customer->demanded = malloc(sizeof(int) * resource_count);
	*customer->allocated = malloc(sizeof(int) * resource_count);
	*customer->needed = malloc(sizeof(int) * resource_count);

	return;
}

void create_resource(Resource *resource) {
	*resource->demanded = malloc(sizeof(int) * customer_count);
	*resource->allocated = malloc(sizeof(int) * customer_count);
	*resource->needed = malloc(sizeof(int) * customer_count);

	return;
}
