#include <stdio.h>
#include <mpi.h>

int MASTER = 0;

typedef enum { LEFT, RIGHT, ABOVE, BELOW } Neighbor;

typedef struct Position {
    int x;
    int y;
} Position;

typedef struct MatrixElement {
    Position a;
    Position b;
} MatrixElement;

//given a certain grid position, size, and direction,
//moves the position, wrapping around if needed
Position neighbor(Position pos, int N, Neighbor direction, int delta) {
    //determine direction
    if(direction == LEFT || direction == ABOVE)
        delta *= -1;

    int newX = pos.x;
    int newY = pos.y;

    //move
    if(direction == LEFT || direction == RIGHT)
        newX += delta;
    else
        newY += delta;

    //normalize

    //if past left or top
    if(newX < 0)
        newX = N + newX;
    if(newY < 0)
        newY = N + newY;

    //if past right or bottom
    if(newX >= N)
        newX = newX % N;
    if(newY >= N)
        newY = newY % N;

    return (Position) { .x = newX, .y = newY };
}

//retrieves the processor ID for a certain position
int valueAtPos(Position pos, int grid[3][3]) {
    return grid[pos.y][pos.x];
}

void buildProcessorGrid(int N, int grid[3][3]) {
    int currId = 1;
    for(int x = 0; x < N; x++) {
        for(int y = 0; y < N; y++) {
            grid[y][x] = currId++;
        }
    }
}

Position findProcessor(int id, int N, int grid[3][3]) {
    for(int x = 0; x < N; x++) {
        for(int y = 0; y < N; y++) {
            Position currPos = { .x = x, .y = y };

            if(valueAtPos(currPos, grid) == id)
                return currPos;
        }
    }

    return (Position) { .x = -1, .y = -1 };
}

void printGrid(int grid[3][3], int N) {
    for(int x = 0; x < N; x++) {
        for(int y = 0; y < N; y++) {
            printf("%d ", grid[y][x]);
        }
        printf("\n");
    }
}

void LOG(char* message, int ownId) {
    printf("Processor %d: %s\n", ownId, message);
}

void initAlign(MatrixElement initialValues[3][3], int matrixA[3][3], int matrixB[3][3], int N) {
    for(int x = 0; x < N; x++) {
        for(int y = 0; y < N; y++) {
            Position currPos = { .x = x, .y = y };

            MatrixElement initialValue = {
                .a = neighbor(currPos, N, RIGHT, y),
                .b = neighbor(currPos, N, BELOW, x)
            };
            initialValues[y][x] = initialValue;
        }
    }
}

void gather(int result[3][3], int N, int numProcessors, int processorGrid[3][3]) {
    for(int i = 0; i < numProcessors; i++) {
        //[0] is result, [1] is source
        int resultMessage[2];

        LOG("Waiting for result...", MASTER);
        MPI_Recv(&resultMessage, 2, MPI_INT, MPI_ANY_SOURCE, 2, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

        //figure out which position the sender is in
        Position receivedFrom = findProcessor(resultMessage[1], N, processorGrid);

        printf("Processor %d: Got a result from #%d at (%d, %d)\n",
            MASTER, resultMessage[1], receivedFrom.x, receivedFrom.y);

        //assign the result to the appropriate position
        result[receivedFrom.y][receivedFrom.x] = resultMessage[0];
    }
}

void getElements(int* elementA, int* elementB, int fromA, int fromB) {
    MPI_Recv(elementA, 1, MPI_INT, fromA, 2, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
    MPI_Recv(elementB, 1, MPI_INT, fromB, 2, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
}

void sendElements(int elementA, int elementB, int toA, int toB) {
    MPI_Send(&elementA, 1, MPI_INT, toA, 2, MPI_COMM_WORLD);
    MPI_Send(&elementB, 1, MPI_INT, toB, 2, MPI_COMM_WORLD);
}

//sets up the matricies, processor grid, and sends starting data to processors
void initMaster(int N, int processorIds[3][3]) {
    LOG("Processor Grid", MASTER);
    printGrid(processorIds, N);

    LOG("Setting up matricies...", MASTER);

    int matrixA[3][3] = {
        {3, 5, 2},
        {7, 3, 6},
        {1, 4, 5}
    };

    int matrixB[3][3] = {
        {2, 3, 7},
        {4, 9, 3},
        {7, 3, 4}
    };

    LOG("Matrix A", MASTER);
    printGrid(matrixA, N);
    LOG("Matrix B", MASTER);
    printGrid(matrixB, N);

    //ok so first master node does the initial allocation
    //wait no dammit we can't do that, what if the dataset is huge

    //ok, so, master just sends initial values at each position to processors
    //and then they're responsible for the initial alignment phase

    //FUCK IT

    LOG("Aligning matricies...", MASTER);

    MatrixElement initialValues[3][3];
    initAlign(initialValues, matrixA, matrixB, N);

    //then, right before you send out the initial matrix elements,
    //send to each node the neighboring processor IDs

    //wait no don't need to send, can just construct. much simpler.
    for(int x = 0; x < N; x++) {
        for(int y = 0; y < N; y++) {
            Position currProcessor = { .x = x, .y = y };
            int currProcessorId = valueAtPos(currProcessor, processorIds);

            MatrixElement value = initialValues[currProcessor.y][currProcessor.x];

            int elementA = valueAtPos(value.a, matrixA);
            int elementB = valueAtPos(value.b, matrixB);
            printf("Processor %d: Sending initial elements [%d, %d] to #%d at (%d, %d)\n",
                MASTER, elementA, elementB, currProcessorId, x, y);
            sendElements(elementA, elementB, currProcessorId, currProcessorId);
        }
    }
    LOG("Gathering...", MASTER);

    int result[3][3];
    gather(result, N, N*N, processorIds);

    LOG("Result", MASTER);
    printGrid(result, N);
}

int calculate(int N, int ownId, int processorIds[3][3]) {
    LOG("Determining neighbors...", ownId);
    printGrid(processorIds, N);

    Position ownPosition = findProcessor(ownId, N, processorIds);
    printf("Processor %d: Position (%d, %d)\n",
        ownId, ownPosition.x, ownPosition.y);


    int neighbors[4] = {
        valueAtPos(neighbor(ownPosition, N, LEFT, 1), processorIds),
        valueAtPos(neighbor(ownPosition, N, RIGHT, 1), processorIds),
        valueAtPos(neighbor(ownPosition, N, ABOVE, 1), processorIds),
        valueAtPos(neighbor(ownPosition, N, BELOW, 1), processorIds)
    };

    LOG("Neighbors", ownId);
    printf("Processor %d: LEFT: %d\n",
        ownId, neighbors[LEFT]);
    printf("Processor %d: RIGHT: %d\n",
        ownId, neighbors[RIGHT]);
    printf("Processor %d: ABOVE: %d\n",
        ownId, neighbors[ABOVE]);
    printf("Processor %d: BELOW: %d\n",
        ownId, neighbors[BELOW]);

    //MAIN PHASE
    int elementA, elementB;

    LOG("Waiting for initial elements...", ownId);

    //receive initial elements
    getElements(&elementA, &elementB, MASTER, MASTER);

    int total = 0;
    for(int i = 0; i < N; i++) {
        LOG("Running calculation...", ownId);

        //THE ACTUAL FUCKING LOGIC FINALLY
        total += elementA * elementB;
        //THATS IT GUYS ITS OVER GO HOME

        //if there's only one iteration
        //left, don't shift
        if(i == N-1)
            break;

        LOG("Sending elements onward...", ownId);
        printf("Processor %d: A: #%d B: #%d\n",
            ownId, neighbors[LEFT], neighbors[ABOVE]);

        sendElements(elementA, elementB, neighbors[LEFT], neighbors[ABOVE]);

        LOG("Waiting for next elements...", ownId);
        getElements(&elementA, &elementB, neighbors[RIGHT], neighbors[BELOW]);
    }

    LOG("Done calculating, notifying master", ownId);

    int resultMessage[2] = { total, ownId };
    MPI_Send(resultMessage, 2, MPI_INT, MASTER, 2, MPI_COMM_WORLD);

    return total;
}

void cannon(int argc, char **argv) {
    MPI_Init(&argc, &argv);

    int ownId, numProcessors;

    MPI_Comm_rank(MPI_COMM_WORLD, &ownId);
    MPI_Comm_size(MPI_COMM_WORLD, &numProcessors);

    int N = 3;

    LOG("Initializing...", ownId);

    int processorIds[3][3];
    buildProcessorGrid(N, processorIds);

    LOG("Built processor grid", ownId);

    if(ownId == MASTER) {
        initMaster(N, processorIds);
        LOG("Master done", ownId);
    }
    else {
        int result = calculate(N, ownId, processorIds);
        //shit out some results
        printf("Processor %d: Got result: %d\n", ownId, result);
    }

    MPI_Finalize();
}

int main(int argc, char **argv)
{
    cannon(argc, argv);
    return 0;
}
