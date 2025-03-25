#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <readline/readline.h>
#include <readline/history.h>
#include <time.h>
#include "lambda_calc.h"


static error_t parse_opt(int key, char *arg, struct argp_state *state) {
    struct arguments *arguments = state->input;

    switch (key) {
        case 'v': arguments->verbose = 1; break;
        case 'f': arguments->load_file = arg; break;
        case ARGP_KEY_END: break;
        default: return ARGP_ERR_UNKNOWN;
    }
    return 0;
}

char* add_prefix(const char* prefix, const char* str) {
    size_t prefix_len = strlen(prefix);
    size_t str_len = strlen(str);
    size_t total_len = prefix_len + str_len + 1; // +1 for null terminator

    char* result = malloc(total_len);
    if (!result) {
        fprintf(stderr, "Memory allocation failed!\n");
        return NULL;
    }

    strcpy(result, prefix);    // Copy prefix first
    strcat(result, str);       // Then append the original string

    return result;
}

char* generate_random_string(char spacer,size_t length) {
    if (length == 0) return NULL; // Safety check

    char* random_string = malloc(length + 2); // Allocate memory (+1 for null terminator)
    if (!random_string) {
        perror("Memory allocation failed");
        exit(EXIT_FAILURE);
    }

    size_t charset_size = sizeof(CHARSET) - 1;
    random_string[0] = spacer;
    for (size_t i = 1; i < length + 1; i++) {
        random_string[i] = CHARSET[rand() % charset_size]; // Pick a random character
    }

    random_string[length + 1] = '\0'; // Null terminate the string
    return random_string;
}

void free_token(BaseToken* token){
    if (!token) return;

    if(token->type == 0 || token->type == 1){
        free(token->var_name);
    }
    
    // Only loop through existing children
    for (int i = 0; i < 2; i++){
        if (token->in_values[i]) {
            free_token(token->in_values[i]);
            token->in_values[i] = NULL;
        }
    }
    free(token);
    token = NULL;
}

unsigned int hash(const char* str) {
    unsigned int hash = 5381;
    while (*str)
        hash = ((hash << 5) + hash) + *str++;  // DJB2 hash algorithm
    return hash % TABLE_SIZE;
}

HashVarriable** get_all_variable_entries(HashTable* ht, int* count) {
    *count = 0;  // Initialize count

    // Count the number of stored variables
    for (int i = 0; i < TABLE_SIZE; i++) {
        HashVarriable* entry = ht->table[i];
        while (entry) {
            (*count)++;
            entry = entry->next;
        }
    }

    if (*count == 0) return NULL;  // No variables found

    // Allocate space for variable entries
    HashVarriable** entries = malloc((*count) * sizeof(HashVarriable*));
    if (!entries) {
        perror("Failed to allocate memory for variable list");
        exit(EXIT_FAILURE);
    }

    int index = 0;
    for (int i = 0; i < TABLE_SIZE; i++) {
        HashVarriable* entry = ht->table[i];
        while (entry) {
            entries[index] = entry;  // Store the pointer to the entry
            index++;
            entry = entry->next;
        }
    }

    return entries;  // Return list of `HashVarriable*`
}

void insert_variable(HashTable* ht, const char* name, BaseToken* value) {
    unsigned int index = hash(name);
    HashVarriable* entry = ht->table[index];

    // Check if variable already exists and replace it
    while (entry) {
        if (strcmp(entry->name, name) == 0) {
            // Free old value and replace it
            free_token(entry->value);  // Free previous BaseToken (if needed)
            entry->value = value;
            return;  // Exit after replacing
        }
        entry = entry->next;
    }

    // If not found, create a new variable entry
    HashVarriable* newVar = malloc(sizeof(HashVarriable));
    newVar->name = strdup(name);
    newVar->value = value;
    newVar->next = ht->table[index];  // Collision handling via chaining
    ht->table[index] = newVar;
}

BaseToken* get_variable(HashTable* ht, const char* name) {
    unsigned int index = hash(name);
    HashVarriable* entry = ht->table[index];
    while (entry) {
        if (strcmp(entry->name, name) == 0)
            return entry->value;
        entry = entry->next;
    }
    return NULL;  // Variable not found
}

BaseToken* clone_base_token(BaseToken* token) {
    if (!token) return NULL;  // Handle NULL input

    // Allocate memory for the new token
    BaseToken* newToken = malloc(sizeof(BaseToken));
    if (!newToken) {
        perror("Failed to allocate memory for new BaseToken");
        exit(EXIT_FAILURE);
    }

    // Copy primitive data
    newToken->type = token->type;
    newToken->var_len = token->var_len;

    // Copy variable name (allocate memory first)
    if (token->var_name) {
        token->var_len = strlen(token->var_name);
        newToken->var_name = malloc(token->var_len + 1); // +1 for null terminator
        if (!newToken->var_name) {
            perror("Failed to allocate memory for var_name");
            exit(EXIT_FAILURE);
        }
        strncpy(newToken->var_name, token->var_name, token->var_len);
        newToken->var_name[token->var_len] = '\0'; // Ensure null termination
    } else {
        newToken->var_name = NULL;
    }

    // Recursively clone children (if present)
    for (int i = 0; i < 2; i++) {
        newToken->in_values[i] = clone_base_token(token->in_values[i]);
    }

    return newToken;
}

void free_table(HashTable* ht) {
    for (int i = 0; i < TABLE_SIZE; i++) {
        HashVarriable* entry = ht->table[i];
        while (entry) {
            HashVarriable* temp = entry;
            entry = entry->next;
            free(temp->name);
            free_token(temp->value);
            free(temp);
        }
    }
    free(ht);
}

void parse_str(char** input, BaseToken** token){
    if ((*input)[0] == '('){
        if((*input)[1] == '\\'){
            (*input) += 2;
            while (strcspn(*input, " ") < strcspn(*input, "."))
            {
                (*token)->type = 1;
                (*token)->var_len = strcspn(*input, " ");
                (*token)->var_name = malloc(sizeof(char) * (*token)->var_len + 1);
                strncpy((*token)->var_name, *input, (*token)->var_len);
                (*token)->var_name[(*token)->var_len] = '\0';
                *input += (*token)->var_len + 1;
                (*token)->in_values[0] = malloc(sizeof(BaseToken));
                memset((*token)->in_values[0], 0, sizeof(BaseToken)); 
                token = &(*token)->in_values[0];
            }
            

            (*token)->type = 1;
            (*token)->var_len = strcspn(*input, ".");
            (*token)->var_name = malloc(sizeof(char) * (*token)->var_len + 1);
            strncpy((*token)->var_name, *input, (*token)->var_len);
            (*token)->var_name[(*token)->var_len] = '\0';
            
            *input += (*token)->var_len + 1;
            (*token)->in_values[0] = malloc(sizeof(BaseToken));
            memset((*token)->in_values[0], 0, sizeof(BaseToken)); 
            parse_str(input, &(*token)->in_values[0]);
            (*input)++; // Skip parenthasis at the end
        }else{
            (*input)++;
            (*token)->type = 2;
            (*token)->in_values[0] = malloc(sizeof(BaseToken));
            memset((*token)->in_values[0], 0, sizeof(BaseToken)); 
            (*token)->in_values[1] = malloc(sizeof(BaseToken));
            memset((*token)->in_values[1], 0, sizeof(BaseToken)); 
            parse_str(input, &(*token)->in_values[0]);
            parse_str(input, &(*token)->in_values[1]);
            while ((*input)[0] != ')')
            {
                BaseToken* parentToken = malloc(sizeof(BaseToken));
                memset(parentToken, 0, sizeof(BaseToken));
                parentToken->type = 2;
                parentToken->in_values[0] = *token;
                parentToken->in_values[1] = malloc(sizeof(BaseToken));
                memset(parentToken->in_values[1], 0, sizeof(BaseToken));
                parse_str(input, &parentToken->in_values[1]);
                (*token) = parentToken;
            }
            
            (*input)++;
        }
    }else{
        (*token)->type = 0;
        (*token)->var_len = strcspn(*input, " ()");
        (*token)->var_name = malloc(sizeof(char) * (*token)->var_len + 1);
        strncpy((*token)->var_name, *input, (*token)->var_len);
        (*token)->var_name[(*token)->var_len] = '\0';
        (*input) += (*token)->var_len;
        while(**input == ' ') (*input)++;
    }
}

void print_parse(BaseToken* token){
    char* varName;
    if(token->type == 1 || token->type == 0){
        int varLen = strcspn(token->var_name, ":");
        varName = malloc(sizeof(char) * varLen + 1);
        strncpy(varName, token->var_name, varLen);
        varName[varLen] = '\0';
    }
    if(token->type == 1){
        printf("(\\%s.",varName);
        print_parse(token->in_values[0]);
        printf(")");
    }
    if(token->type == 0){
        printf("%s",varName);
    }
    if(token->type == 2){
        printf("(");
        print_parse(token->in_values[0]);
        if(token->in_values[0]->type == 0 && token->in_values[1]->type == 0) printf(" ");
        print_parse(token->in_values[1]);
        printf(")");
    }
}

int add_recursive_suffix(BaseToken* token, char* suffix){
    int ret = 0;
    if(token->type == 0 || token->type == 1){
        size_t original_len = strlen(token->var_name);
        size_t suffix_len = strlen(suffix);
        size_t new_len = original_len + suffix_len + 1;

        char* new_var_name = realloc(token->var_name, new_len);
        if (!new_var_name) {
            perror("Memory allocation failed");
            exit(EXIT_FAILURE);
        }
    
        token->var_name = new_var_name;
        strcat(token->var_name, suffix);
        ret = 1;
    }

    for (int i = 0; i < token->type; i++){
        if (add_recursive_suffix(token->in_values[i], suffix))
            ret = 1;
    }
    return ret;
}

int beta_reduction_rec(BaseToken** token, char* varriable, BaseToken* value){
    if((*token)->type == 0){
        //printf("Testing %s == %s result %d\n",varriable, (*token)->var_name, strcmp(varriable, (*token)->var_name) == 0);
        if (strcmp(varriable, (*token)->var_name) == 0){
            free_token(*token);

            (*token) = clone_base_token(value);
        }
    }
    else{
        for (int i = 0; i < (*token)->type; i++){
            beta_reduction_rec(&((*token)->in_values[i]), varriable, value);
        }
    }
    
    return 1;
}

void beta_reduction(BaseToken** token){
    if (!(*token) || (*token)->type != 2) return;

    BaseToken* func = (*token)->in_values[0];
    BaseToken* value = (*token)->in_values[1];

    if (!func) return;

    beta_reduction_rec(&func->in_values[0], func->var_name, value);

    free_token(value);

    // Assign cloned token
    *token = func->in_values[0];
}

int beta_reduction_search(BaseToken** token){
    if((*token)->type == 2){
        if((*token)->in_values[0]->type == 1){
            beta_reduction(token);
            return 1;
        }
        int out = beta_reduction_search(&(*token)->in_values[0]);
        if (out == 0)
            out = beta_reduction_search(&(*token)->in_values[1]);
        return out;
    }    
    if((*token)->type == 1)
        return beta_reduction_search(&(*token)->in_values[0]);
    return 0;
}

void print_map_varriables(HashTable* table){
    int count;
    HashVarriable** varriables = get_all_variable_entries(table, &count);
    for(int i =0; i< count; i++){
        printf("%s    ", varriables[i]->name);
        print_parse(varriables[i]->value);
        printf("\n");
    }
}

int expand_varriable(BaseToken** token, HashTable* table){
    int ret = 0;

    
    for (int i = 0; i < (*token)->type; i++){
        if (expand_varriable(&((*token)->in_values[i]), table))
            ret = 1;
    }

    if((*token)->type == 0){
        int varLen = strcspn((*token)->var_name, ":");
        char* varName = malloc(sizeof(char) * varLen + 1);
        strncpy(varName, (*token)->var_name, varLen);
        varName[varLen] = '\0';
        BaseToken* var = get_variable(table, varName);
        if (var != NULL){
            free_token(*token);
            (*token) = clone_base_token(var);
            add_recursive_suffix((*token), generate_random_string(':',16));
            ret = 1;
        }
    }
    return ret;
}

void remove_newline(char* str) {
    size_t len = strlen(str);
    if (len > 0 && str[len - 1] == '\n') {
        str[len - 1] = '\0';  // Replace '\n' with null terminator
    }
}

void execute_file(const char* filename, HashTable* table) {
    FILE* file = fopen(filename, "r");
    if (!file) {
        perror("Error opening file");
        exit(EXIT_FAILURE);
    }

    char* line = NULL;  // Pointer for dynamic allocation
    size_t len = 0;     // Stores allocated buffer size
    ssize_t read;       // Stores the length of the read line

    while ((read = getline(&line, &len, file)) != -1) {
        remove_newline(line);
        BaseToken* token = command_interpeter(line, table);
        if(token != NULL) free_token(token);
    }

    free(line);  // Free allocated memory
    fclose(file);
}

int token_equal(BaseToken* eq1, BaseToken* eq2){
    if(eq1->type != eq2->type) return 0;

    if(eq1->type == 0 || eq1->type == 1){
        int varLen1 = strcspn(eq1->var_name, ":");
        int varLen2 = strcspn(eq2->var_name, ":");
    
        if(varLen1 != varLen2) return 0;
    
        char* varName1 = malloc(sizeof(char) * varLen1 + 1);
        strncpy(varName1, eq1->var_name, varLen1);
        varName1[varLen1] = '\0';
    
        char* varName2 = malloc(sizeof(char) * varLen2 + 1);
        strncpy(varName2, eq2->var_name, varLen2);
        varName2[varLen2] = '\0';
    
        if(strcmp(varName1, varName2) != 0) return 0;    
    }

    for (int i = 0; i < eq1->type; i++){
        if (!token_equal(eq1->in_values[i], eq2->in_values[i])) return 0;
    }
    return 1;
}

int contract_varriable(BaseToken** token ,HashTable* table){
    int count;
    HashVarriable** allVarriables = get_all_variable_entries(table, &count);
    if (allVarriables == NULL) return 0;

    for(int i = 0;i < count; i++){
        if(token_equal((*token), allVarriables[i]->value)){
            free_token((*token));
            BaseToken* varToken = malloc(sizeof(BaseToken));
            memset(varToken, 0, sizeof(BaseToken));
            varToken->type = 0;
            varToken->var_name = malloc(strlen(allVarriables[i]->name) + 1);
            strcpy(varToken->var_name, allVarriables[i]->name);
            (*token) = varToken;
            return 1;
        }
    }
    int out = 0;
    for (int i = 0; i < (*token)->type; i++){
        out = contract_varriable(&(*token)->in_values[i], table) | out;
    }
    return out;
}

BaseToken* command_interpeter(char* command, HashTable* table){

    size_t currentCommandLength = strcspn(command, " =");

    char* currentCommand = malloc(sizeof(char) * currentCommandLength + 1);
    strncpy(currentCommand, command, currentCommandLength);
    currentCommand[currentCommandLength] = '\0';

    if(strcmp(currentCommand, "br") == 0){
        command += 2;
        while(*command == ' ') command++;
        char* end;
        int br_count = strtol(command, &end, 10);
        if (command == end) br_count = 100;
        command = end;
        while(*command == ' ') command++;

        BaseToken* token = command_interpeter(command, table);
        for (int i = 0; i < br_count; i++){
            int found = beta_reduction_search(&token);
            if(!found)
                break;
        }

        free(currentCommand);
        return token;
    }

    if(strcmp(currentCommand, "ex") == 0){
        command += 2;
        while(*command == ' ') command++;
        char* end;
        int br_count = strtol(command, &end, 10);
        if (command == end) br_count = 1000;
        command = end;
        while(*command == ' ') command++;

        BaseToken* token = command_interpeter(command, table);
        for (int i = 0; i < br_count; i++){
            if(!expand_varriable(&token, table))
                break;
        }

        return token;
    }

    if(strcmp(currentCommand, "con") == 0){
        command += 3;
        while(*command == ' ') command++;
        char* end;
        int br_count = strtol(command, &end, 10);
        if (command == end) br_count = 1000;
        command = end;
        while(*command == ' ') command++;

        BaseToken* token = command_interpeter(command, table);
        for (int i = 0; i < br_count; i++){
            if(!contract_varriable(&token, table))
                break;
        }

        return token;
    }

    if(strcmp(currentCommand, "load") == 0){
        command += 4;
        while(*command == ' ') command++;

        size_t varNameLength = strcspn(command, " =");

        char* varName = malloc(sizeof(char) * varNameLength + 1);
        strncpy(varName, command, varNameLength);
        varName[varNameLength] = '\0';
        command += varNameLength;

        execute_file(varName, table);
    }

    if(strcmp(currentCommand, "def") == 0){
        command += 3;
        while(*command == ' ') command++;

        size_t varNameLength = strcspn(command, " =");

        char* varName = malloc(sizeof(char) * varNameLength + 1);
        strncpy(varName, command, varNameLength);
        varName[varNameLength] = '\0';
        command += varNameLength;
        while(*command == ' ') command++;

        BaseToken* token = command_interpeter(command, table);
        insert_variable(table, varName, token);

        BaseToken* errorToken = malloc(sizeof(BaseToken));
        memset(errorToken, 0, sizeof(BaseToken)); 
        errorToken->var_name = strdup("Created Varriable");
        errorToken->type = 0;
        errorToken->var_len = strlen(errorToken->var_name);
        return errorToken;
    }

    if(strcmp(currentCommand, "show") == 0){
        print_map_varriables(table);

        BaseToken* errorToken = malloc(sizeof(BaseToken));
        memset(errorToken, 0, sizeof(BaseToken)); 
        errorToken->var_name = strdup("Varriable Table");
        errorToken->type = 0;
        errorToken->var_len = strlen(errorToken->var_name);
        return errorToken;
    }

    free(currentCommand);

    
    BaseToken* rootToken = malloc(sizeof(BaseToken));
    memset(rootToken, 0, sizeof(BaseToken)); 
    char *input_cpy = command;
    parse_str(&input_cpy, &rootToken);
    return rootToken;
}

int input_loop(arguments args){
    char *input = NULL;
    srand(time(NULL));
    HashTable table;
    memset(&table, 0, sizeof(HashTable));

    if(args.load_file){
        BaseToken* token = command_interpeter(add_prefix("load ", args.load_file), &table);
        if(token != NULL){
            free_token(token);
        }
    }


    while (1)
    {
        input = readline(" > ");
        if (strcmp(input, "exit\n") == 0) {
            printf("Exiting program...\n");
            break;
        }
        BaseToken* token = command_interpeter(input, &table);
        if(token != NULL){
            print_parse(token);
            free_token(token);
            printf("\n");
        }
 

        add_history(input);
        free(input);
    }
    return 1;
}

int main(int argc, char* argv[]){
    arguments args = {0};

    argp_parse(&argp, argc, argv, 0, 0, &args);

    return input_loop(args);
}
