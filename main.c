#include <stdio.h>
#include "charUtils.h"
#include <string.h>
#include <sys/stat.h>
#include <time.h>
#include <stdbool.h>

#define MAXSTRING 500 //limit Rabo is 50
#define ACCOUNTDEBUG "NL12RABO3456789012"
#define AMOUNTOFSLOTS 19 //rabo uses 19 slots



/*	Remco Pronk, 26-07-16 
	Converts the bankstatement format of the Rabobank to a csv-file that is readable by YNAB */

/* inputfile format

12 columns of importance
account# (IBAN) || Currency || Date (yyyymmdd) || Debet/Credit || value (3.00) || receiver account# (IBAN) ||
Receiver (Name) || Date || type (ba, bg, etc) || ??? (empty) || description || comment

Example 1:
"NL12RABO3456789012","EUR","20140318","D","3.00","","","20140318","ba","","Rest Bernoulliborg GRONINGEN","Betaalautomaat 14:42 pasnr. 004","","","","","","",""

Example 2:
"NL12RABO3456789012","EUR","20140320","C","8.84","NL98RABO7654321012","HEKSTRA J A","20140320","cb","","examenbundel","","","","","","SCT2014031937154000000002001","",""

*/

/* outputDatumfile format:

6 colums, with first line being how the csv-file is formatted

Date,Payee,Category,Memo,Outflow,Inflow
date (dd/mm/yyyy) || from/to || category (YNAB) || comment || outflow (100.00) || inflow (50.00)

Example 1 result:
"NL12RABO3456789012","EUR","20140318","D","3.00","","","20140318","ba","","Rest Bernoulliborg GRONINGEN","Betaalautomaat 14:42 pasnr. 004","","","","","","",""
18/03/2014, Rest Bernoulliborg GRONINGEN, , Betaalautomaat 14:42 pasnr. 004, 3.00, ,

Example 2 result:
"NL12RABO3456789012","EUR","20140320","C","8.84","NL98RABO7654321012","HEKSTRA J A","20140320","cb","","examenbundel","","","","","","SCT2014031937154000000002001","",""
20/03/2014, HEKSTRA J A, , examenbundel, , 8.84

*/

/* Replaces the airquotes from a string with a whitespaces */
char *removeQuotes(char string[]) {
    int counter = ccStrLength(string);

    while (counter > -1) {
        if (string[counter] == '"') {
            string[counter] = ' ';
            //TODO remove whitespaces, check code job
        }
        counter--;
    }

    return string;
}


void reformatString(char input[], FILE *outputFilePointer) {
    int i; // iterator variable
    char seperatedInput[AMOUNTOFSLOTS][MAXSTRING]; // location to save the seperated data (date, amount, etc) from the input.
    //TODO empty array at start //  memset(members, 0, sizeof members);



    //skip the first quotationmark
    input++;
    for (i = 0; i < AMOUNTOFSLOTS; i++) {

        sscanf(input, "%[^'\"']", seperatedInput[i]);
        input = strchr(input, '\"');
        input += 3;
        //printf("Next input will be %s\n", input);
    }


    /*
    for(i = 0; i < AMOUNTOFSLOTS; i++){
        printf("Inputslot %d is %s\n", i, seperatedInput[i]);
    }
     */


    if (strcmp(ACCOUNTDEBUG, seperatedInput[0]) == 0) { //exact match
        //print to file
        //Output order is different for credit and debet
        if (seperatedInput[3][0] == 'C') {
            printf("(Datum) %s || (Naam gever) %s || (Categorie) || (Comment) %s%s || (Outflow) || (Inflow) %s\n",
                   seperatedInput[2], seperatedInput[6], seperatedInput[10], seperatedInput[11], seperatedInput[4]);
            //We don't fill in the category slot
            fprintf(outputFilePointer, "%s,%s,,%s%s,,%s\n", seperatedInput[2], seperatedInput[6], seperatedInput[10],
                    seperatedInput[11], seperatedInput[4]);
    }
        if (seperatedInput[3][0] == 'D') {
            printf("(Datum) %s || (Naam ontvanger) %s || (Categorie) || (Comment) %s%s || (Outflow) %s || (Inflow) \n",
                   seperatedInput[2], seperatedInput[6], seperatedInput[10], seperatedInput[11], seperatedInput[4]);
            fprintf(outputFilePointer, "%s,%s,,%s%s,%s,\n", seperatedInput[2], seperatedInput[6], seperatedInput[10],
                    seperatedInput[11], seperatedInput[4]);
        }
    }


}

void readInput(FILE *ifp, FILE *ofp) {
    char storage[MAXSTRING];

    //TODO other order of printing as well
    //Print first line
    fprintf(ofp, "Date,Payee,Category,Memo,Outflow,Inflow\n");

    do {
        fscanf(ifp, "%[^\n]", storage);
        //Rabobank transaction files end with an empty line, we catch that here
        if (ccStrLength(storage) == 1) {
            return;
        }
        //reformat the the line we just read
        reformatString(storage, ofp);

    } while (fgets(storage, MAXSTRING, ifp) != NULL); //scan next line

    //Done with the file, close it.
    fclose(ifp);
}

void stopProgramAfterInput() {
    printf("\nThe application has finished. Press any key to continue.");
    getchar(); //to not close application without user interaction
    exit(0);
}

void readSettings() {

    //TODO universeel maken door slot aan te laten geven welke info er zit

    char unused[MAXSTRING];
    char line[MAXSTRING];
    //TODO read ini file for settings

    FILE *fpSettings = fopen("settingsYNABConverter.ini", "r+"); //Allow reading AND writing
    if (fpSettings) {
        printf("Setting file exists.\n");

    }
    else { //No file exists
        printf("No setting file exists, creating one...\n");
        //create new ini file
        FILE *opSettings = fopen("settingsYNABConverter.ini", "w");

        //populate file
        fprintf(opSettings, "AccountnumberToUse=\nFormatting=");

        printf("Settingfile created in the directory of the program named 'settingsYNABConverter.ini'. Manually edit the file to add an accountnumber to use with this program.\n");
        stopProgramAfterInput();
    }
}

//TODO clean up main: make functions
int main(int argc, char *argv[]) {
    //reserve memory for input and output
    FILE *ifp;
    FILE *ofp;




    //In case of no inputfile given, the program can not work.
    if (argv[1] == NULL) {
        printf("There was no inputfile.\nDrag and drop a file unto the executable to process it.\n");
        stopProgramAfterInput();
    }

    readSettings();


    //Open the inputfile and save it to inputFilePointer
    ifp = fopen(argv[1], "r");
    //Create a file to write to: outputFilePointer
    ofp = fopen("output.csv", "w");
    //read, parse and reformat the input
    readInput(ifp, ofp);
    //close the output file
    fclose(ofp);


    //end the application
    exit(0);

}
