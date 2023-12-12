#include <stdlib.h>
#include<stdint.h>
#include <stdio.h>
#include <string.h>

//Use this value to alter the size of the dictionary
#define DICT_SIZE 256

struct DictionaryEntry{
    uint8_t key;
    uint8_t* value;
    int value_size;
};
typedef struct DictionaryEntry Entry;

void populate_dictionary(Entry* dict);
void destroy_dictionary(Entry* dict);
uint8_t* append_byte(const uint8_t* src1,int size,uint8_t src2);
int is_pattern_in_dict(const Entry* dict,const uint8_t* pattern, const int* size);
uint8_t get_key_for_pattern(const uint8_t* pattern,int size, const Entry* dict);
int byte_compress(unsigned char* data_ptr, int data_size);
void display_test(const char* name, uint8_t* data, int size);

int main() {

    //Main is being used to test program output

    uint8_t ptr1[] = {0x03, 0x74, 0x04, 0x04, 0x04, 0x35, 0x35, 0x64,0x64, 0x64, 0x64, 0x00, 0x00, 0x00, 0x00, 0x00,0x56, 0x45, 0x56, 0x56, 0x56, 0x09, 0x09, 0x09};
    int ptr1_size = 24;
    display_test("ptr1",ptr1,ptr1_size);

    uint8_t ptr2[] = {'t','h','i','s','i','s','t','h','e'};
    int ptr2_size = 9;
    display_test("ptr2",ptr2,ptr2_size);

    uint8_t ptr3[] = {'t','h','i','s','t','h','i','s',};
    int ptr3_size = 8;
    display_test("ptr3",ptr3,ptr3_size);

    return 0;
}


/*
 * Populates dict 0x00 - 0x1F key value pairs
 *0x20 - DICT_SIZE are empty value keys to be used for patterns stored
 *
 * requirements: dict be an Entry array of size DICT_SIZE
 * */
void populate_dictionary(Entry* dict){

    uint8_t current_key = 0x00;
    for(int i = 0; i < DICT_SIZE; i++){
        //Initialize a key for each possible entry in the dictionary
        dict[i].key = current_key;
        dict[i].value_size = 1;
        //Each potential value in our input (0x00 - 0x1F) gets its value set as itself.
        if(i < 128) {
            dict[i].value = (uint8_t *) malloc(sizeof(uint8_t));

            if(dict[i].value == NULL){
                perror("Memory Allocation failed");
                exit(EXIT_FAILURE);
            }
            *dict[i].value = current_key;
        }
        else{
            //Set uninitialized values to NULL for later checking
            dict[i].value = NULL;
        }
        current_key++;
    }

}
/*
 * Deallocates all memory used in the dictionary
 * */
void destroy_dictionary(Entry* dict){

    for(int i = 0; i < DICT_SIZE; i++){
        //If we reached NULL entries, we are done deallocating
        if(dict[i].value == NULL){
            break;
        }
        else{
            free(dict[i].value);
        }
    }
}

/*
 * Appends a byte to an existing byte array,
 * returns a new dynamically allocated byte array of src1 appended with src2
 *
 * requirements: src1 to be a pointer to a byte array, and src2 to be a byte
 * requirements: size to be the size of the new array, the length of src1 + 1
 * */

uint8_t* append_byte(const uint8_t* src1,int size,uint8_t src2){

    uint8_t* new_array = (uint8_t*)malloc(sizeof(uint8_t)*(size));
    if(new_array == NULL){
        perror("Memory Allocation failed");
        exit(EXIT_FAILURE);
    }

    for(int i = 0; i < size; i++){
        //Copy current pattern into the check, with the last byte being next byte
        if(i + 1 == size){
            new_array[i] = src2;
        }
        else{
            new_array[i] = src1[i];
        }
    }
    return new_array;
}

/*
 * Checks if a given pattern exists in a dictionary of Entries. Returns 1 if it is found, 0 otherwise
 *
 * requirements: dict to be an array of Entry structures, pattern to be a pointer to a byte array,
 *              and size to be a pointer to an integer representing the size of the pattern.
 *              The dictionary must be properly initialized and contain valid data.
 *
 *
 */

int is_pattern_in_dict(const Entry* dict,const uint8_t* pattern, const int* size) {

    for (int j = 128; j < DICT_SIZE; j++) {
        //If we reach indexes with NULL values, then we didn't find the pattern in the dictionary
        if (dict[j].value == NULL) {
            return 0;
        }
            //If the size of the patterns are not equal than they cannot be the same
        else if (*size != dict[j].value_size) {
            continue;
        }
            //If the size of the patterns are equal,we can use memcmp to directly compare them
        else if (memcmp(pattern, dict[j].value, *size) == 0) {
            return 1;
        }
    }
}

/*
 * Retrieves the key associated with a given pattern from a dictionary of Entries.
 * This function assumes we know the pattern is in the dictionary. It will throw an error if pattern is not found
 *
 * requirements: pattern to be a pointer to a byte array, size to be the size of the pattern,
 *              and dict to be an array of Entry structures. The dictionary must be properly initialized
 *              and contain valid data.
 *
 */
uint8_t get_key_for_pattern(const uint8_t* pattern,int size, const Entry* dict){

    for (int i = 0; i < DICT_SIZE; i++) {
        //Only need to compare if pattern size and dict value size are the same
        if(size == dict[i].value_size){
            for(int j = 0; j < size; i++) {
                //If it any point the pattern, doesn't match, continue search
                if (pattern[j] != dict[i].value[j]) {
                    continue;
                }
                return dict[i].key;
            }
        }
    }
    perror("get_key_for_pattern was called, but pattern was not in dictionary");
    exit(EXIT_FAILURE);
}

/*
 * Compresses a byte array using an LZW compression algorithm,updates data_ptr with the compressed data
 *
 * requirements: data_ptr to be a pointer to a byte array, data_size to be the size of the byte array.
 *
 * Returns the size of the compressed data.
 */
int byte_compress(unsigned char* data_ptr, int data_size){

    //Write output into buffer until we can determine its length
    uint8_t buffer[500] = {0};
    int buffer_index = 0;

    //Algorithm requires at least 3 bytes to potentially perform any compression
    if(data_size < 3){
        return data_size;
    }

    //Creating our dictionary with all 127 possible single byte values 0x00 - 0x1F
    //The rest of the entries are empty and are used for storing patterns
    Entry Dictionary[DICT_SIZE];
    populate_dictionary(Dictionary);
    //We will use this later when we need to add to our indexes
    int next_available_index = 128;

    //We start our current pattern as the first data value
    uint8_t* current_pattern = (uint8_t*) malloc(sizeof (uint8_t));
    current_pattern[0] = data_ptr[0];
    int current_pattern_size = 1;

    //Iterate for each data value in the input
    for(int i = 1; i < data_size; i++){

        //This will be the next value our pattern is concerned with
        uint8_t next_value = data_ptr[i];

        //Our new potential pattern is current pattern concatenated with the next value
        int pattern_to_check_size = current_pattern_size + 1;
        uint8_t* pattern_to_check = append_byte(current_pattern,pattern_to_check_size,next_value) ;

        // If our pattern is in the dictionary already, add the next value to the current pattern and start over
        if(is_pattern_in_dict(Dictionary,pattern_to_check,&pattern_to_check_size)){

            free(current_pattern);
            current_pattern = pattern_to_check;
            current_pattern_size = pattern_to_check_size;
        }
            //If our pattern + next byte is not in the dictionary, we add to the dictionary
        else{
            //Add the key of the pattern to our output
            buffer[buffer_index] = get_key_for_pattern(current_pattern, current_pattern_size, Dictionary);
            buffer_index++;

            //add pattern to dictionary
            Dictionary[next_available_index].value = pattern_to_check;
            Dictionary[next_available_index].value_size = pattern_to_check_size;
            next_available_index++;

            //Move current pattern pointer to next value
            free(current_pattern);
            current_pattern = (uint8_t*)malloc(sizeof (int8_t));
            current_pattern[0] = next_value;
            current_pattern_size = 1;
        }
    }

    //At this point, we have, no more next values to worry about
    //We still have one more current pattern that must be added

    //If the last pattern is one byte, we can simply add it
    if(current_pattern_size == 1){
        buffer[buffer_index] = current_pattern[0];
    }
        //Otherwise, we need to find the key value for the pattern
    else{
        buffer[buffer_index] = get_key_for_pattern(current_pattern, current_pattern_size, Dictionary);
    }
    buffer_index++;
    //Buffer now has compressed string, we can simply read it into the original string and return the number of bytes

    for(int i = 0; i < buffer_index; i++){
        data_ptr[i] = buffer[i];
    }

    free(current_pattern);
    destroy_dictionary(Dictionary);

    return buffer_index;
}

/*
 * Displays the number of data along with the data itself before and after running the compression algorithm
 * */
void display_test(const char* name, uint8_t* data, int size){

    printf("\nNumber of bytes in %s before compression: %d\n",name,size);
    printf("Byte array before compression: ");
    for(int i = 0; i < size;i++){
        printf(" %02X ",data[i]);
    }

    int new_size = byte_compress(data,size);

    printf("\nNumber of bytes in %s after compression: %d\n",name,new_size);
    printf("New byte array: ");

    for(int i = 0; i < new_size;i++){
        printf(" %d ",data[i]);
    }
    printf("\n");


}