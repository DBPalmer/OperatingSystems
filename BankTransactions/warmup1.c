/**
 * Created by Lucas (Deuce) Palmer
*/
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/time.h>
#include <locale.h>
#include <ctype.h>
#include <time.h>

#include "cs402.h"
#include "my402list.h"

//structure to store a bank transaction
typedef struct tagTransactions {
    time_t date;
    char description[25];
    int cents;
} Transaction;

//function declarations
int ReadInput(FILE *, My402List*);
void SortInput(My402List*);
void BubbleForward(My402List *, My402ListElem **, My402ListElem **);
void PrintStatement(My402List*);
void PrintNum(int);
int DigitCount(int);

int main(int argc, char *argv[])
{
    FILE *fp;
    if (argc == 1){//no arguments
        fprintf(stderr, "Incorrect command line input. Must pass arguments to ./warmup1.\n");
        exit(1);
    }
    if (strcmp(argv[1], "sort") == 0){
        if (argc == 2){//if no file specified, read from stdin
            fp = stdin;
            if (fp == NULL){
                fprintf(stderr, "Failed to read from stdin.\n");
                exit(1);
            }
        } else if (argc == 3){//read from file
            fp = fopen(argv[2], "r");
            if (fp == NULL){
                fprintf(stderr, "Failed to open input file.\n");
                perror(argv[2]);
                exit(1);
            }
        } else {//incorrect number of arguments
            fprintf(stderr, "Incorrect command line input. Invalid number of arguments.\n");
            exit(1);
        }
    } else {//user did not type "sort"
        fprintf(stderr, "Incorrect command line input. \"%s\" is not a valid option.\n", argv[1]);
        exit(1);
    }
    My402List list;
    if (!My402ListInit(&list)) {
        fprintf(stderr, "Failed to initialize list of transactions.\n");
        exit(1);
    } 
    if (!ReadInput(fp, &list)) {
        //multiple formatting errors possible
        //error print outs inside ReadInput function
        exit(1);
    } 
    if (fp != stdin){//if file was opened, it must be closed
        fclose(fp);
    }
    SortInput(&list);
    PrintStatement(&list);
    return 0;
}
int ReadInput(FILE *fp, My402List* list){
    time_t now = time(0);
    char buf[2000];
    int line_cnt = 1;
    while (fgets(buf, sizeof(buf), fp) != NULL) {//reads till end of file
        if (strlen(buf) > 1024){//a line longer than 1024 characters is not exceptable
            fprintf(stderr, "Invalid input file format, line %d of input file is too long.\n", line_cnt);
            return 0;
        }
        Transaction *tran = (Transaction*)malloc(sizeof(Transaction));//create a Transaction
        //read the line up to the first '\t'
        char *start_ptr = buf;
        char *tab_ptr = strchr(start_ptr, '\t');
        char type;
        if (tab_ptr != NULL){
            *tab_ptr++ = '\0';
            type = start_ptr[0];
        } else {//'\t' was not found, indicating the file is invalid as it does not have the proper format
            fprintf(stderr, "Invalid input file format, not enough fields at line %d of input file.\n", line_cnt);
            return 0;
        }
        if (type != '+' && type != '-'){//check transaction type
            fprintf(stderr, "Invalid transaction type at line %d of input file.\n", line_cnt);
            return 0;
        }
        //read the line up to the second '\t'
        start_ptr = tab_ptr;
        tab_ptr = strchr(start_ptr, '\t');
        if (tab_ptr != NULL){
            *tab_ptr++ = '\0';
            if (((time_t) atoi(start_ptr)) > 0 && ((time_t) atoi(start_ptr)) < now && DigitCount(((time_t) atoi(start_ptr))) >= 10){
                tran->date = (time_t) atoi(start_ptr);
            } else {//indicates invalid timestamp, per the errors defined in spec
                fprintf(stderr, "Invalid timestamp at line %d of input file.\n", line_cnt);
                return 0;
            }
        } else {//'\t' was not found, indicating the file is invalid as it does not have the proper format
            fprintf(stderr, "Invalid input file format, not enough fields at line %d of input file.\n", line_cnt);
            return 0;
        }
        //read the line up to the third '\t'
        start_ptr = tab_ptr;
        tab_ptr = strchr(start_ptr, '\t');
        if (tab_ptr != NULL){
            *tab_ptr++ = '\0';
            int isdecimal = 0;
            int decimals = 0;
            //counts the amount of decimals in number
            for (int i=0; i<strlen(start_ptr); i++){
                if (isdecimal == 1){
                    decimals++;
                }
                if (start_ptr[i] == '.'){
                    isdecimal = 1;
                }
            }
            if (decimals != 2){//returns error if there are not two decimals
                fprintf(stderr, "Invalid transaction decimal precision at line %d of input file.\n", line_cnt);
                return 0;
            }
            float amount = atof(start_ptr);
            if (amount < 0){//per spec, "The transaction amount must have a positive value"
                fprintf(stderr, "Transaction must be positive at line %d of input file.\n", line_cnt);
                return 0;
            }
            if (type == '+'){//if the type is a deposit
                tran->cents = (int)(round(amount*100));//make positive
            } else {//if the type is withdrawal
                tran->cents = -(int)(round(amount*100));//make negative
            }
        } else {//'\t' was not found, indicating the file is invalid as it does not have the proper format
            fprintf(stderr, "Invalid input file format, not enough fields at line %d of input file.\n", line_cnt);
            return 0;
        }
        //read the rest of the line
        start_ptr = tab_ptr;
        tab_ptr = strchr(start_ptr, '\t');
        if (tab_ptr == NULL){
            if (start_ptr[strlen(start_ptr)-1] == '\n'){//remove '\n'
                start_ptr[strlen(start_ptr)-1] = '\0';
            }
            while(isspace((unsigned char)*start_ptr)){//take out lead spaces
                start_ptr++;
            }
            memcpy(tran->description, start_ptr, 25);//copy only the first 25 characters (to fit in print out)
            int i = strlen(start_ptr);
            if (i == 0){//per spec, "After leading space characters have been removed, a transaction description must not be empty"
                fprintf(stderr, "Empty description at line %d of input file.\n", line_cnt);
                return 0;
            }
            while (i < 25){//in case the description is less than 25 characters, fill with spaces to simplify later print out
                tran->description[i++] = ' ';
            }
            tran->description[25-1] = '\0';
        } else {//'\t' was found, indicating that there are more than 3 '\t's per line
            fprintf(stderr, "Invalid input file format, too many fields at line %d of input file.\n", line_cnt);
            return 0;
        }
        line_cnt++;
        if (!My402ListAppend(list, (void*)tran)){//add Transaction entry to My402List
            fprintf(stderr, "Failed to add Transaction to list\n");
            return 0;
        }
    }
    if (line_cnt == 1){//a valid file needs at least one entry
        fprintf(stderr, "Invalid input file format, no transactions.\n");
        return 0;
    }
    return 1;
}
void SortInput(My402List* list){
    //code modified from function given in listtest.c
    My402ListElem *elem=NULL;
    int i=0;
    int num_items = My402ListLength(list);
    for (i=0; i < num_items; i++) {
        int j=0, something_swapped=FALSE;
        My402ListElem *next_elem=NULL;

        for (elem=My402ListFirst(list), j=0; j < num_items-i-1; elem=next_elem, j++) {
            Transaction cur_val= *(Transaction *)(elem->obj);

            next_elem= My402ListNext(list, elem);
            Transaction next_val= *(Transaction *)(next_elem->obj);

            if (cur_val.date > next_val.date) {//compare timestamps
                BubbleForward(list, &elem, &next_elem);
                something_swapped = TRUE;
            } else if (cur_val.date == next_val.date){//cannot have two identical timestamps
                fprintf(stderr, "ERROR, two bank transactions cannot ocurr at the same time.\n");
                exit(1);
            }
        }
        if (!something_swapped) break;
    }
}
void BubbleForward(My402List *list, My402ListElem **pp_elem1, My402ListElem **pp_elem2)
    /* (*pp_elem1) must be closer to First() than (*pp_elem2) */
{
    //directy from provided function in listtest.c
    My402ListElem *elem1=(*pp_elem1), *elem2=(*pp_elem2);
    void *obj1=elem1->obj, *obj2=elem2->obj;
    My402ListElem *elem1prev=My402ListPrev(list, elem1);
    My402ListElem *elem2next=My402ListNext(list, elem2);

    My402ListUnlink(list, elem1);
    My402ListUnlink(list, elem2);
    if (elem1prev == NULL) {
        (void)My402ListPrepend(list, obj2);
        *pp_elem1 = My402ListFirst(list);
    } else {
        (void)My402ListInsertAfter(list, obj2, elem1prev);
        *pp_elem1 = My402ListNext(list, elem1prev);
    }
    if (elem2next == NULL) {
        (void)My402ListAppend(list, obj1);
        *pp_elem2 = My402ListLast(list);
    } else {
        (void)My402ListInsertBefore(list, obj1, elem2next);
        *pp_elem2 = My402ListPrev(list, elem2next);
    }
}
void PrintStatement(My402List* list){
    int balance = 0;
    printf("+-----------------+--------------------------+----------------+----------------+\n");
    printf("|       Date      | Description              |         Amount |        Balance |\n");
    printf("+-----------------+--------------------------+----------------+----------------+\n");
    My402ListElem *iter = list->anchor.next;
    //iterate list and print each transaction
    while (iter != &list->anchor){
        Transaction transact = *(Transaction*)(iter->obj);
        printf("| %.*s", 10, (char*)ctime(&transact.date));
        printf(" %.*s |", 4, (char*)ctime(&transact.date)+20);
        printf(" %s |", transact.description);
        PrintNum(transact.cents);
        balance += transact.cents;
        PrintNum(balance);
        printf("\n");
        iter = iter->next;
    }
    printf("+-----------------+--------------------------+----------------+----------------+\n");
}
void PrintNum(int num){
    if (num * .01 >= 10000000){//too many characters
        printf("  ?,???,???.??  |");
    } else if (num * .01 <= -10000000){
        printf(" (?,???,???.\?\?) |");//add parenthesis for negative number
    } else {
        int length = DigitCount(num);
        if (abs(num) >= 100000){//handles extra characters for commas
            length ++;
            if (abs(num) >= 100000000){//will have 2 commas
            length++;
        }
        }
        if (num == 0){//handles proper output for transactions < $1.00
            length--;
        } else if (abs(num) < 100){
            length++;
            if (abs(num) < 10){
                length++;
            }
        }
        //for loop to pad front to make numbers align to the right
        for (int i=0; i<(12-length); i++){
            if (i ==1 && num < 0){
                printf("(");//add parenthesis for negative number
            } else {
                printf(" ");
            }
        }
        setlocale(LC_NUMERIC, "");//used to print commas
        if (num == 0){//per spec, "if the number is zero, its length must be 1"
            printf("%d", num);
        } else {//cents changed to dollar amount for final print out
            printf(" %'.2f", abs(num) * .01);
        }
        if(num < 0){
            printf(") |");//add parenthesis for negative number
        } else {
            printf("  |");
        }
    }
}
int DigitCount(int num){
    //effectively strlen() for an integer
    int i = 0;
    while (num != 0){
        num = num/10;
        i++;
    }
    return i;
}