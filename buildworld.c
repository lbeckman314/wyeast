// author: liam beckman
// date: 09 may 2018
// cs344 block 2 program
// sources:
//   cs344 lectures and resources
//   https://ubuntuforums.org/showthread.php?t=1430052
//   https://stackoverflow.com/questions/2674312/how-to-append-strings-using-sprintf

#include <fcntl.h>      // create files
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>   // mkdir
#include <time.h>       // random generator
#include <unistd.h>     // process id

// maximum name length (includes null terminator, but not newline)
#define MAX_NAME 9

// Room definition
typedef struct Room Room;
struct Room
{
    int id;
    char* name;
    int numOutboundConnections;
    struct Room* outboundConnections[6];
    char* type;
};


// function prototypes
void AddRandomConnection();
bool CanAddConnectionFrom(Room x);
void ConnectRoom(Room x, Room y);
bool ConnectionAlreadyExists(Room x, Room y);
Room GenerateRoom();
Room GetRandomRoom();
bool IsGraphFull();
bool IsSameRoom(Room x, Room y);
void WriteFiles();


// names
char* realNames[10] = {"chinook", "klamath", "llao", "loowit", "mazama", "pahto", "sahale", "umatilla", "umpqua", "wyeast"};

// debugging room names
// char* realNames[10] = {"a", "b", "c", "d", "e", "f", "g", "h", "i", "j"};

int selectedRoomNames[7] = {-1, -1, -1, -1, -1, -1, -1};

// types
char* realTypes[3] = {"START_ROOM", "MID_ROOM", "END_ROOM"};
int selectedRoomTypes[7] = {-1, -1, -1, -1, -1, -1, -1};

// connections
int outboundConnections[7][7];

// number of rooms generated
int roomsGenerated = 0;

// array of rooms
struct Room roomArray[7];


int main()
{
    // seed the random generator
    srand(time(NULL));

    while(roomsGenerated <= 6)
    {
        GenerateRoom();
    }

    // Create all connections in graph
    while (IsGraphFull() == false)
    {
        AddRandomConnection();
    }

    // write to room files
    WriteFiles();
}


// Returns true if all rooms have 3 to 6 outbound connections, false otherwise
bool IsGraphFull()
{
    int outbound;
    int i;

    // for all selected rooms, check to see if the number of outbound connections
    // fulfill the requirements
    for (i = 0; i < 7; i++)
    {
        outbound = roomArray[i].numOutboundConnections;

        if (outbound < 3 || outbound > 6)
        {
            return false;
        }
    }

    return true;

}


// Adds a random, valid outbound connection from a Room to another Room
void AddRandomConnection()
{
    Room A;  // Maybe a struct, maybe global arrays of ints
    Room B;

    // continue adding connections to rooms until they fulfill al requirements
    while(true)
    {
        A = GetRandomRoom();

        if (CanAddConnectionFrom(A) == true)
        {
            break;
        }
    }

    do
    {
        B = GetRandomRoom();
    } while(CanAddConnectionFrom(B) == false || IsSameRoom(A, B) == true || ConnectionAlreadyExists(A, B) == true);

    ConnectRoom(A, B);
    ConnectRoom(B, A);
}


// return a random room from the room array
Room GetRandomRoom()
{
    int randomInt = rand();

    Room randomRoom;

    // get random room from room array
    randomRoom = roomArray[randomInt % 7];

    return randomRoom;
}


// Returns a random Room, does NOT validate if connection can be added
Room GenerateRoom()
{
    // get random integer
    int randomInt = rand();

    // initialize our new room
    Room new;

    // id
    new.id = roomsGenerated;

    // get unique name
    bool uniqueName = false;
    while(!uniqueName)
    {
        randomInt = rand();
        selectedRoomNames[roomsGenerated] = randomInt % 10;
        new.name = realNames[selectedRoomNames[roomsGenerated]];

        // presume the name is unique.
        uniqueName = true;

        int i;
        for(i = 0; i < roomsGenerated; i++)
        {
            // if the name matches a previous one, then rerun.
            if(selectedRoomNames[i] == selectedRoomNames[roomsGenerated])
            {
                uniqueName = false;
            }
        }
    }

    // set name
    new.name = realNames[randomInt % 10];

    // outbound connections
    new.numOutboundConnections = 0;

    // set type
    if (roomsGenerated == 0)
    {
        // START_ROOM
        selectedRoomTypes[roomsGenerated] = 0;
    }

    else if (roomsGenerated == 6)
    {
        // MID_ROOM
        selectedRoomTypes[roomsGenerated] = 2;
    }

    else
    {
        // END_ROOM
        selectedRoomTypes[roomsGenerated] = 1;
    }

    new.type = realTypes[selectedRoomTypes[roomsGenerated]];

    roomArray[roomsGenerated] = new;
    roomsGenerated += 1;

    return new;
}


// Returns true if a connection can be added from Room x (< 6 outbound connections), false otherwise
bool CanAddConnectionFrom(Room x)
{
    if (x.numOutboundConnections < 6)
    {
        return true;
    }

    return false;
}


// Returns true if a connection from Room x to Room y already exists, false otherwise
bool ConnectionAlreadyExists(Room x, Room y)
{
    if (outboundConnections[x.id][y.id] == 1)
    {
        return true;
    }

    return false;
}


// Connects Rooms x and y together, does not check if this connection is valid
void ConnectRoom(Room x, Room y)
{
    outboundConnections[x.id][y.id] = 1;

    roomArray[x.id].outboundConnections[x.numOutboundConnections] = &roomArray[y.id];

    roomArray[x.id].numOutboundConnections += 1;
}


// Returns true if Rooms x and y are the same Room, false otherwise
bool IsSameRoom(Room x, Room y)
{
    if (x.id == y.id)
    {
        return true;
    }

    return false;
}


// writes a room file with following format:
// ROOM NAME: <room name>
// CONNECTION 1: <room name>
// …
// CONNECTION 6: <room name>
// ROOM TYPE: <room type>
void WriteFiles()
{
    // get process id
    int processID = getpid();
    char processIDString[10];
    memset(processIDString, '\0', 10);
    sprintf(processIDString, "%d", processID);

    // directory prefix
    char roomDirString[20];
    memset(roomDirString, '\0', 20);
    strcpy(roomDirString, "beckmanl.rooms.");

    // make directory
    strcat(roomDirString, processIDString);
    int roomDir = mkdir(roomDirString, 0755);


    // for all rooms, write all room contents to file
    int i;
    for (i = 0; i < 7; i++)
    {
        char roomIDString[10];
        memset(roomIDString, '\0', 10);
        sprintf(roomIDString, "%d", roomArray[i].id);

        char roomFileString[20];
        memset(roomFileString, '\1', 20);

        // room name
        strcpy(roomFileString, roomArray[i].name);

        // room prefix ("_room")
        strcat(roomFileString, "_room");

        char filepath[30];
        memset(filepath, '\0', 30);
        strcpy(filepath, roomDirString);
        strcat(filepath, "/");
        strcat(filepath, roomFileString);

        int file_descriptor;
        ssize_t nread, nwritten;
        char writeBuffer[200];

        // open (or create) room file
        file_descriptor = open(filepath, O_WRONLY | O_CREAT, 0600);

        if (file_descriptor < 0)
        {
            fprintf(stderr, "Could not open %s\n", filepath);
            perror("Error in main()");
            exit(1);
        }

        // write room NAME to buffer
        sprintf(writeBuffer, "ROOM NAME: %s\n", roomArray[i].name);

        // write room CONNECTIONS to buffer
        int n;
        for (n = 0; n < roomArray[i].numOutboundConnections; n++)
        {
            // append to string
            sprintf(writeBuffer + strlen(writeBuffer), "CONNECTION %d: %s\n", n + 1, roomArray[i].outboundConnections[n]->name);
        }

        // write room TYPE to buffer
        sprintf(writeBuffer + strlen(writeBuffer), "ROOM TYPE: %s\n", roomArray[i].type);

        // write buffer to room file
        nwritten = write(file_descriptor, writeBuffer, strlen(writeBuffer) * sizeof(char));

        close(file_descriptor);
    }
}
