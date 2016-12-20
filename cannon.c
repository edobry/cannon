#include <stdio.h>
#include <mpi.h>

int MASTER = 0;

typedef enum { LEFT, RIGHT, TOP, BOTTOM } Neighbor;

typedef struct Position {
    int x;
    int y;
} Position;

//given a certain grid position, size, and direction,
//moves the position, wrapping around if needed
Position neighbor(Position pos, int N, Neighbor direction) {
    //determine direction
    int delta = 1;
    if(direction == LEFT || direction == TOP)
        delta = -1;

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
        newX = N - newX;
    if(newY < 0)
        newY = N - newY;

    //if past right or bottom
    if(newX > N)
        newX -= N;
    if(newY > N)
        newY -= N;

    return { .x = newX, .y = newY };
}

//retrieves the processor ID for a certain position
int valueAtPos(Position currpos, int* grid[3][3]) {
    return grid[currpos.x][currpos.y];
}

//sets up the matricies, processor grid, and sends starting data to processors
void initMaster() {
    int N = 3;

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

    //ok so first master node does the initial allocation
    //wait no dammit we can't do that, what if the dataset is huge

    //ok, so, master just sends initial values at each position to processors
    //and then they're responsible for the initial alignment phase


    //figure out all the processor IDs and put them in this nice grid
    int processorIds[3][3] = buildProcessorGrid(N, MPI_COMM_WORLD);


    //then, right before you send out the initial matrix elements,
    //send to each node the neighboring processor IDs
    for(int i = 0; i < N; i ++) {
        for(int j = 0; j < N; j++) {
            Position currProcessor = { .x = i, .y = j };
            int currProcessorId = valueAtPos(currProcessor, processorIds);

            //oh wait actually we need to tell the nodes where they're
            //recieving values from too, right... so thats 4 now, lets use an
            //array, order will be [left right top bottom]
            int neighbors[4] = {
                valueAtPos(neighbor(currProcessor, N, LEFT), processorIds),
                valueAtPos(neighbor(currProcessor, N, RIGHT), processorIds),
                valueAtPos(neighbor(currProcessor, N, TOP), processorIds),
                valueAtPos(neighbor(currProcessor, N, BOTTOM), processorIds)
            };

            //send the neighbors
            MPI_Send(&neighbors, 4, MPI_INT, currProcessorId, 2, MPI_COMM_WORLD);

            //send the initial matrix elements
            int elements[2] = {
                valueAtPos(currProcessor, matrixA),
                valueAtPos(currProcessor, matrixB)
            };
            MPI_Send(&elements, 2, MPI_INT, currProcessorId, 2, MPI_COMM_WORLD);
        }
    }
}

int calculate(int N) {
    int neighbors[4];
    MPI_Recv(&neighbors, 4, MPI_INT, MASTER, 2, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

    //ALIGNMENT PHASE
    for()
    //wait, how do i figure out my own id...


    //MAIN PHASE
    int total = 0;
    for(int i = 0; i < N; i++) {
        int elementA, elementB;
        MPI_Recv(&elementA, 1, MPI_INT, neighbors[RIGHT], 2, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        MPI_Recv(&elementB, 1, MPI_INT, neighbors[BELOW], 2, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

        total += currentElements[0] * currentElements[1];

        MPI_Send(&elementA, 1, MPI_INT, neighbors[LEFT], 2, MPI_COMM_WORLD);
        MPI_Send(&elementB, 1, MPI_INT, neighbors[ABOVE], 2, MPI_COMM_WORLD);
    }

    return total;
}

int main (int argc, char **argv)
{
    MPI_Init(&argc, &argv);
    int mypid, nprocs;

    MPI_Comm_rank(MPI_COMM_WORLD, &mypid);
    MPI_Comm_size(MPI_COMM_WORLD, &nprocs);

    int N = 3;

    int result;
    if(mypid == MASTER)
        initMaster(N);
    else
        result = calculate(N);

    /* shit out some results */

    MPI_Finalize();
    return 0;
}
