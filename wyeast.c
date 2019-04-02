// author: liam beckman
// date: 09 may 2018
// cs344 block 2 program
// sources:
//   cs344 lectures and resources
//   https://stackoverflow.com/questions/19342155/how-to-store-characters-into-a-char-pointer-using-the-strcpy-function
//   https://www.geeksforgeeks.org/c-program-list-files-sub-directories-directory/
//   https://linux.die.net/man/3/strftime
//   https://www.poetryfoundation.org/poetrymagazine/browse?contentId=20403
//   https://www.poetryfoundation.org/poetrymagazine/browse?contentId=24060

#include <assert.h>
#include <dirent.h>     // find most recent directory
#include <fcntl.h>      // create files
#include <pthread.h>    // threads
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>   // find most recent directory
#include <time.h>
#include <unistd.h>     // process id

// used in recording user's path
#define ALLOWED_TURNS 100

// maximum name length (includes null terminator, but not newline)
#define MAX_NAME 9

// Room definition
typedef struct Room Room;
struct Room
{
    int id;
    char* name;
    int numOutboundConnections;

    // modified outbound array to hold only room names
    // instead of room pointers
    char* outbound[6];
    char* type;
};


// function prototypes
void  allFiles(char* filepath);
void  clean();
char* getRecentDir();
Room  getRooms(char* filepath);
void* getTime();
int   hasWon(Room current);
Room  playerInterface(Room current, pthread_t timeThread);
void  updatePath(Room current);


// number of rooms generated
int roomsGenerated = 0;

// room names
char* realNames[10] = {"chinook", "klamath", "llao", "loowit", "mazama", "pahto", "sahale", "umatilla", "umpqua", "wyeast"};

// debugging room names
// char* realNames[10] = {"a", "b", "c", "d", "e", "f", "g", "h", "i", "j"};

// filenames of room files
char* fileNames[7];

// array of rooms
struct Room roomArray[7];

// number of steps player takes
int steps = 0;

// user path,
char *path[ALLOWED_TURNS];

// create mutex (used in getting current time)
pthread_mutex_t myMutex = PTHREAD_MUTEX_INITIALIZER;


int main()
{
    // lock mutex
    pthread_mutex_lock(&myMutex);

    // create time thread
    pthread_t timeThread;
    int result_code;
    result_code = pthread_create(&timeThread, NULL, getTime, NULL);
    assert(0 == result_code);

    // get the most recent rooms directory
    char recentDir[20];
    memset(recentDir, '\0', 20);
    strcpy(recentDir, getRecentDir());

    // set current directory in a string variable to reuse
    char roomFilePrefix[30];
    memset(roomFilePrefix, '\0', 30);
    strcpy(roomFilePrefix, recentDir);
    strcat(roomFilePrefix, "/");

    // initialize the next room (the room the user selects)
    Room roomNext;

    // initialize the starting room
    Room roomCurrent;

    // set the starting room filepath
    char roomFile[30];
    memset(roomFile, '\0', 30);
    strcpy(roomFile, roomFilePrefix);

    // initialize all rooms from room files
    allFiles(roomFile);

    // starting room
    roomCurrent = roomArray[0];

    // show all rooms (good for debugging)
    /*
    int n;
    for (n = 0; n < 7; n++)
    {
        printf("roomArray[%d]: %s :: ", n, roomArray[n].type);
        printf("%s :: ", roomArray[n].name);
        printf("%d\n", roomArray[n].id);
    }
    */

    // display current room and prompt
    // while the player hasn't reached the end room
    do
    {
        // update the player prompt
        roomNext = playerInterface(roomCurrent, timeThread);

        // reset the pathname
        strcpy(roomFile, roomFilePrefix);

        // update the current room
        roomCurrent = roomNext;

        // updated the number of steps and room path
        updatePath(roomCurrent);

    } while (hasWon(roomCurrent) == 0);


    // winning message
    printf("YOU HAVE FOUND THE END ROOM. CONGRATULATIONS!\n");
    printf("YOU TOOK %d STEPS. ", steps);
    printf("YOUR PATH TO VICTORY WAS:\n");

    int i;
    for (i = 0; i < steps; i++)
    {
        printf("%s\n", path[i]);
    }

    // free data
    clean();

    // When your program exits, set the exit status code to 0, and
    // leave the rooms directory in place, so that it can be examined.
    return 0;
}


// return the most recently modified rooms directory
// rooms directories will be named "beckmanl.rooms.12345"
// where "12345" represents the running process's id.
char* getRecentDir()
{
    char* dirPath = ".";
    char* dirPrefix = "beckmanl.rooms.";

    DIR *d;
    struct dirent *dir;
    d = opendir(".");

    time_t max;
    max = -1;

    if (d)
    {
        // for all beckmanl.rooms.* directories in current directory
        while ((dir = readdir(d)) != NULL)
        {
            if (strstr(dir->d_name, dirPrefix) != NULL)
            {
                // perform stat() function call
                struct stat sb;
                stat(dir->d_name, &sb);

                // open the most recent st_mtime component of the returned stat struct
                if (sb.st_mtime > max)
                {
                    max = sb.st_mtime;
                    dirPath = dir->d_name;
                }
            }
        }

        closedir(d);
    }

    // return most recent directory
    return dirPath;
}


// iterate over all room files in the newest room directory
// calls getRooms() function on all room files.
void allFiles(char* filepath)
{
    // Pointer for directory entry
    struct dirent *dirEntry;

    // opendir() returns a pointer of DIR type.
    DIR *dirRead = opendir(filepath);

    // opendir returns NULL if couldn't open directory
    if (dirRead == NULL)
    {
        printf("Could not open directory");
        exit(1);
    }

    // for all files in the most recently modified directory
    int count = 0;
    while ((dirEntry = readdir(dirRead)) != NULL)
        // don't include current or parent directories/filenames
        if (strcmp (dirEntry->d_name, ".")  != 0
        &&  strcmp (dirEntry->d_name, "..") != 0)
        {
            // add filename to fileNames array
            fileNames[count] = malloc(sizeof(char) * MAX_NAME);
            strcpy(fileNames[count], dirEntry->d_name);

            // increment count
            count += 1;
        }

    closedir(dirRead);


    // call getRooms() seven times to populate rooms array.
    int i;
    for (i = 0; i < 7; i++)
    {
        int file_descriptor;

        file_descriptor = open(filepath, O_RDONLY);

        if (file_descriptor < 0)
        {
            fprintf(stderr, "Could not open %s\n", filepath);
            perror("Error in main()");
            exit(1);
        }

        getRooms(filepath);

        close(file_descriptor);
    }
}


// populate roomArray with contents of room files
Room getRooms(char* filepath)
{
    // add directory to filepath
    char file[50];
    memset(file, '\0', 50);
    strcat(file, filepath);

    // add filename to filepath
    char id[10];
    sprintf(id, "%d", roomsGenerated);
    strcat(file, fileNames[roomsGenerated]);

    // remove newline from end of filepath
    int len = strlen(file);
    if (len > 0 && file[len - 1] == '\n')
        file[--len] = '\0';

    // open room file in room directory
    FILE* myFile;
    myFile = fopen(file, "r");

    if (myFile < 0)
    {
        fprintf(stderr, "Could not open %s\n", file);
        exit(1);
    }

    int numCharsEntered = -1;
    size_t bufferSize = 0;
    char* lineEntered = NULL;

    // initialize new room
    Room myRoom;
    myRoom.numOutboundConnections = 0;

    // get name
    numCharsEntered = getline(&lineEntered, &bufferSize, myFile);
    char mystring[20];
    memset(mystring, '\0', 20);
    strcpy(mystring, lineEntered);

    // token = "ROOM NAME: a"
    char* token = strtok(mystring, ":");

    // token = "a"
    token= strtok(NULL, " ");

    myRoom.name = malloc(sizeof(char) * MAX_NAME);
    strcpy(myRoom.name, token);

    // remove newline from end of room's name
    len = strlen(myRoom.name);
    if (len > 0 && myRoom.name[len - 1] == '\n')
        myRoom.name[--len] = '\0';

    // get outbound connections
    while(1)
    {
        memset(mystring, '\0', 20);
        strcpy(mystring, lineEntered);

        numCharsEntered = getline(&lineEntered, &bufferSize, myFile);

        // token = "CONNECTION 1: a"
        token = strtok(mystring, ":");

        // token = "a"
        token= strtok(NULL, " ");

        // remove newline at end of token
        len = strlen(token);
        if (len > 0 && token[len - 1] == '\n')
            token[--len] = '\0';


        // as long as room is not connected to itself,
        // add outbound connection
        if (strcmp(token, myRoom.name) != 0)
        {
            myRoom.outbound[myRoom.numOutboundConnections] = malloc(sizeof(char) * MAX_NAME);
            strcpy(myRoom.outbound[myRoom.numOutboundConnections], token);

            myRoom.numOutboundConnections += 1;
        }

        // if line is not a connection line
        if (strstr(lineEntered, "CONNECTION") == NULL)
            break;

    }

    // get type
    numCharsEntered = getline(&lineEntered, &bufferSize, myFile);
    memset(mystring, '\0', 20);
    strcpy(mystring, lineEntered);

    // token = "ROOM TYPE: START_ROOM"
    token = strtok(mystring, ":");

    // token = "START_ROOM"
    token= strtok(NULL, " ");

    // remove newline at end of room type
    len = strlen(token);
    if (len > 0 && token[len - 1] == '\n')
    {
        token[--len] = '\0';
    }

    myRoom.type = (char *)malloc(20 * sizeof(char));
    memset(myRoom.type, '\0', 20);

    // add type to room
    strcpy(myRoom.type, token);

    // add start room to room array
    if (strcmp(myRoom.type, "START_ROOM") == 0)
    {
        roomArray[0] = myRoom;
        roomArray[0].id = 0;
    }

    // add mid rooms to room array
    else if (strcmp(myRoom.type, "MID_ROOM") == 0)
    {
        int n;
        for (n = 1; n < 6; n++)
        {
            if (roomArray[n].name == NULL)
            {
                roomArray[n] = myRoom;
                roomArray[n].id = n;
                break;
            }
        }
    }

    // add end room to room array
    else if (strcmp(myRoom.type, "END_ROOM") == 0)
    {
        roomArray[6] = myRoom;
        roomArray[6].id = 6;
    }

    roomsGenerated += 1;

    fclose(myFile);

    free(lineEntered);

    // return room
    return myRoom;
}


// display prompts for user and get input
Room playerInterface(Room current, pthread_t timeThread)
{
    // user selected room
    Room nextRoom;

    // boolean switch
    int validRoomName;
    validRoomName = 0;

    // boolean switch
    int timeCalled;

    // user input
    char *buffer;
    size_t bufferSize = MAX_NAME + 1;
    buffer = (char *)malloc(bufferSize * sizeof(char));

    if (buffer == NULL)
    {
        perror("unable to allocate dynamic buffer");
        exit(1);
    }

    // prompt while the user has not entered a valid room name
    while (!validRoomName)
    {
        // set time called switch
        timeCalled = 0;

        // output current location and possible connections
        printf("CURRENT LOCATION: %s\n", current.name);
        printf("POSSIBLE CONNECTIONS: ");

        // print outbound connections (up to last one)
        int i;
        for (i = 0; i < current.numOutboundConnections - 1; i++)
        {
            printf("%s, ", current.outbound[i]);
        }

        // print last outbound connection and period/newline
        printf("%s", current.outbound[current.numOutboundConnections-1]);
        printf(".\n");

        // prompt for next room
        printf("WHERE TO? >");

        getline(&buffer, &bufferSize, stdin);

        // remove next line from input buffer (stdin)
        int len = strlen(buffer);
        if (len > 0 && buffer[len - 1] == '\n')
            buffer[--len] = '\0';

        // if time function is not invoked
        if (strcmp("time", buffer) != 0)
        {
            // search all outbound connections of current room
            for (i = 0; i < current.numOutboundConnections; i++)
            {
                // if one matches, assign the next room
                // to the matching room array element
                if (strcmp(current.outbound[i], buffer) == 0)
                {
                    int n;
                    for (n = 0; n < 7; n++)
                    {
                        if (strcmp(buffer, roomArray[n].name) == 0)
                        {
                            validRoomName = 1;
                            nextRoom = roomArray[n];
                        }
                    }
                }
            }
        }

        // if time function is invoked
        else
        {
            // unlock mutex
            pthread_mutex_unlock(&myMutex);

            // wait for time thread to complete as it calls getTime() function
            int result_code;
            result_code = pthread_join(timeThread, NULL);
            assert(0 == result_code);

            // lock mutex again
            pthread_mutex_lock(&myMutex);

            // create new time thread
            result_code = pthread_create(&timeThread, NULL, getTime, NULL);
            assert(0 == result_code);

            // read current time from time file
            char filepath[20];
            memset(filepath, '\0', 20);
            strcpy(filepath, "currentTime.txt");

            FILE* myFile;
            myFile = fopen(filepath, "r");

            if (myFile < 0)
            {
                fprintf(stderr, "Could not open %s\n", filepath);
                exit(1);
            }

            char readBuffer[50];

            int numCharsEntered = -1;
            size_t bufferSize = 0;
            char* lineEntered = NULL;

            // clear out array before writing to it.
            memset(readBuffer, '\0', sizeof(readBuffer));

            numCharsEntered = getline(&lineEntered, &bufferSize, myFile);

            // output time
            printf("\n%s", lineEntered);

            timeCalled = 1;
        }

        // if neither the time function was called or a room
        // was provided, output error and repeat prompt loop
        if (!validRoomName && !timeCalled)
        {
            printf("\nHUH? I DON’T UNDERSTAND THAT ROOM. TRY AGAIN.\n");
        }

        printf("\n");
    }

    free(buffer);

    return nextRoom;
}


// write current time to "currentTime.txt"
// and return buffer with current time.
void* getTime()
{
    // time thread lock mutex
    pthread_mutex_lock(&myMutex);

    // write buffer
    char* bufferArg;
    bufferArg = malloc(sizeof(char) * 30);
    memset(bufferArg, '\0', 30);

    // current time
    time_t rawtime;
    struct tm * timeinfo;
    time (&rawtime);
    timeinfo = localtime (&rawtime);

    // format time string
    strftime(bufferArg, 80, "%I:%M%P, %A, %B %d, %Y\n", timeinfo);

    // filepath of time file
    char filepath[20];
    memset(filepath, '\0', 20);
    strcpy(filepath, "currentTime.txt");

    int file_descriptor;
    char writeBuffer[50];

    // clear out array before writing to it.
    memset(writeBuffer, '\0', sizeof(writeBuffer));

    // open (or create) file
    file_descriptor = open(filepath, O_WRONLY | O_CREAT, 0600);

    if (file_descriptor < 0)
    {
        fprintf(stderr, "Could not open %s\n", filepath);
        perror("Error in main()");
        exit(1);
    }

    // write to file
    write(file_descriptor, bufferArg, strlen(bufferArg) * sizeof(char));

    // close file
    close(file_descriptor);

    free(bufferArg);

    // time thread unlock mutex
    pthread_mutex_unlock(&myMutex);
}


// update user's path from START_ROOM to END_ROOM
void updatePath(Room current)
{
    // update path (rooms visited)
    path[steps] = malloc(sizeof(char) * MAX_NAME);
    memset(path[steps], '\0', MAX_NAME);
    strcpy(path[steps], current.name);

    // update number of steps taken
    steps += 1;
}


// check to see if user has won or not
int hasWon(Room current)
{
    // if current room is the end room, then the player has won!
    if (strcmp(current.type, "END_ROOM") == 0)
    {
        return 1;
    }

    // else, the game continues.
    return 0;
}


// free up any malloc'd memory for global variables
void clean()
{
    // for all elements in user's path from room to room
    int elements = sizeof(path)/sizeof(path[0]);
    int i;
    for (i = 0; i < elements; i++)
    {
        free(path[i]);
    }

    // for all rooms in the room array and fileNames array
    for (i = 0; i < 7; i++)
    {
        int n;
        for (n = 0; n < roomArray[i].numOutboundConnections; n++)
        {
            free(roomArray[i].outbound[n]);
        }

        free(roomArray[i].type);
        free(roomArray[i].name);
        free(fileNames[i]);
    }
}


/*
~ * ~ * ~ * ~ * ~ * ~ * ~ * ~ * ~
P O E T R Y  of the  M O N T H
~ * ~ * ~ * ~ * ~ * ~ * ~ * ~ * ~

"Song Before Adventure"
    by Henry W. Rago

The air is shattered with our breath,
The startled mountains toss.
Come, there are stars for us to pluck
And skis to swing across.

This path awaits our wild young feet,
The planets glean aflutter:
So much, so much for us to do!

  What makes the candle sputter?

~ * ~ * ~ * ~ * ~ * ~ * ~ * ~ * ~

"Occult Adventure"
    by Dilys Bennet Laing

Through every surface of my skin
the needles of the pines struck in.

The blind trees through my pupils could
see themselves: a winter wood.

With the shared organ of my ear
when the bird cried, the bough could hear.

Sucked dry by every staring bud,
identity forsook my blood.

I was the wood's uprooted ghost
the self, half guessed, of that cold host.

Snow fell to snow. I woke. My skis
shot with my self beyond the trees.

~ * ~ * ~ * ~ * ~ * ~ * ~ * ~ * ~
*/
