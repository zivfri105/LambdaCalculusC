#ifndef LAMBDA_CALC
#define LAMBDA_CALC

#include <sys/types.h>
#include <argp.h>

/*Hash map table size(should be based round the expected number of varriables)*/
#define TABLE_SIZE 128
/*Values for argp*/
const char *argp_program_version = "lambdacalc 0.1";
const char *argp_program_bug_address = "<axowattle@gmail.com>";
static char doc[] = "Lambda Calculus Calculator for linux using C";

static struct argp argp = {options, parse_opt, 0, doc};


/*Basic token of the Lambda*/
typedef struct BaseToken{
    u_int8_t type; // Type 0: varriable, 1: function defention, 2: function execution
    char* var_name; // Name of the stored varriable in the token(used in types 0, 1)
    size_t var_len; // Lenght of the character although most places use strlen 
    struct BaseToken* in_values[2]; // Child values of the token used 1 in type 1 and 2 in type 2
} BaseToken;

/*Hash varriable to store saved varriable names*/
typedef struct HashVarriable
{
    char* name;
    BaseToken* value;
    struct HashVarriable* next;
} HashVarriable;

/*Hash table for varriables*/
typedef struct HashTable {
    HashVarriable* table[TABLE_SIZE];
} HashTable;

/*Options for argp*/
// note: verbose currentl does nothing
static struct argp_option options[] = {
    {"verbose", 'v', 0, 0, "Produce verbose output"},
    {"loadfile",  'f', "FILE", 0, "Load File in the start"},
    {0}
};

/*Given arguments for the current execution*/
typedef struct arguments {
    int verbose;
    char *load_file;
}arguments;

/*Parse the given arguments into the struct*/
static error_t parse_opt(int key, char *arg, struct argp_state *state);

/*Adds two strings together one as a prefix and one as a string*/
char* add_prefix(const char* prefix, const char* str);

#define CHARSET "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789"

/*Function to generate a random string of given size*/
char* generate_random_string(char spacer,size_t length);

/*Free the token pointer and all its children*/
void free_token(BaseToken* token);

/*Hash function for the hasmap*/
unsigned int hash(const char* str);

/*Function to get all HashVarriables in the table*/
HashVarriable** get_all_variable_entries(HashTable* ht, int* count);

/*Insert varriable into hashmap*/
void insert_variable(HashTable* ht, const char* name, BaseToken* value);

/*Retrive a varriable from the hash table by name*/
BaseToken* get_variable(HashTable* ht, const char* name);

/*Copy all data of token and all its children to a new pointer*/
BaseToken* clone_base_token(BaseToken* token);

/*Free hasmap data*/
void free_table(HashTable* ht);

/*A parser to parse string input to a tree describing the lambda functions*/
void parse_str(char** input, BaseToken** token);

/*A parser to parse string input to a tree describing the lambda functions*/
void parse_str(char** input, BaseToken** token);

/*Adds suffix to all the tokens that has varriable and their children*/
int add_recursive_suffix(BaseToken* token, char* suffix);

/*Adds suffix to all the tokens that has varriable and their children*/
int add_recursive_suffix(BaseToken* token, char* suffix);

/*Adds suffix to all the tokens that has varriable and their children*/
int add_recursive_suffix(BaseToken* token, char* suffix);

/*Adds suffix to all the tokens that has varriable and their children*/
int add_recursive_suffix(BaseToken* token, char* suffix);

/*Adds suffix to all the tokens that has varriable and their children*/
int add_recursive_suffix(BaseToken* token, char* suffix);

/*Convert varriable names to their full value*/
int expand_varriable(BaseToken** token, HashTable* table);

/*Convert varriable names to their full value*/
int expand_varriable(BaseToken** token, HashTable* table);

/*Removes \n and the end of lines*/
void remove_newline(char* str);

/*Executes the commands in a file sperated by \n*/
void execute_file(const char* filename, HashTable* table);

/*Checks the equality of two tokens*/
int token_equal(BaseToken* eq1, BaseToken* eq2);

/*Converts a varriable values to varriable name*/
int contract_varriable(BaseToken** token ,HashTable* table);

/*Handles inputs of command and execution of correct functions*/
BaseToken* command_interpeter(char* command, HashTable* table);

/*Input loop to get commands from the user*/
int input_loop(arguments args);

#endif