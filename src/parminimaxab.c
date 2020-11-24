#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <math.h>
#include <mpi.h>
#include <time.h>

const int EMPTY = 0;
const int BLACK = 1;
const int WHITE = 2;
const int OUTER = 3;
const int ALLDIRECTIONS[8]={-11, -10, -9, -1, 1, 9, 10, 11};
const int BOARDSIZE=100;
const int TRUE = 1;
const int FALSE = 0;
const int ALPHA = -20000;
const int BETA = 20000;
const double TIMELIMIT = 3.8;
const double MINTIME = 0.0000004;

/*
 * The below weighted matrix is based off the following paper
 * Szubert, Marcin & Jaśkowski, Wojciech & Krawiec, Krzysztof. (2011). 
 * Learning Board Evaluation Function for Othello by Hybridizing Coevolution with Temporal Difference Learning. 
 * Control and Cybernetics. 40. 805-831. 
 */
const int WEIGHTS[100]={0,   0,   0,   0,   0,   0,   0,   0,   0,  0,
                        0, 101, -43,  38,   7,   0,  42, -20, 102,  0,
                        0, -27, -74, -16, -14, -13, -25, -65, -39,  0,
                        0,  56, -30,  12,   5,  -4,   7, -15,  48,  0,
                        0,   1,  -8,   1,  -1,  -4,  -2, -12,   3,  0,
                        0, -10,  -8,   1,  -1,  -3,   2,  -4, -20,  0,
                        0,  59, -23,   6,   1,   4,   6, -19,  35,  0,
                        0,  -6, -55, -18,  -8, -15, -31, -82, -58,  0,
                        0,  96, -42,  67,  -2,  -3,  81, -51, 101,  0,
                        0,   0,   0,   0,   0,   0,   0,   0,   0,  0};

typedef struct score_data {
    int score;
    int move;
} score_data;

char *gen_move();
void play_move(char *move);
void game_over();
void run_worker();
void initialise();

int *initialboard(void);
int *legalmoves ();
int numlegalmoves(int player, int *board);
int legalp (int move, int player, int *board);
int validp (int move);
int wouldflip (int move, int dir, int player, int *board);
int opponent (int player);
int findbracketingpiece(int square, int dir, int player, int *board);
int serialMinimaxInitialAB(int *board, int player, int depth, double start);
int parallelMinimaxInitialAB(int *board, int move, int player, int depth, double start, double timeleft);
int minimaxRecurseAB(int *board, int player, int alpha, int beta, int depth, double start, double timeleft);
void makemove (int move, int player, int *board);
void makeflips (int move, int dir, int player, int *board);
int get_loc(char* movestring);
char *get_move_string(int loc);
void printboard();
char nameof(int piece);
int count (int player, int * board);

int my_colour;
int time_limit;
int running;
int rank;
int size;
int *globalboard;
int *moves;
int *scores;
int firstrun = 1;
FILE *fp;

int main(int argc , char *argv[]) {
    int socket_desc, port, msg_len;
    char *ip, *cmd, *opponent_move, *my_move;
    char msg_buf[15], len_buf[2];
    struct sockaddr_in server;
    ip = argv[1];
    port = atoi(argv[2]);
    time_limit = atoi(argv[3]);
    my_colour = EMPTY;
    running = 1;

    /* starts MPI */
    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);	/* get current process id */
    MPI_Comm_size(MPI_COMM_WORLD, &size);	/* get number of processes */
    // Rank 0 is responsible for handling communication with the server
    if (rank == 0) {
        fp = fopen(argv[4], "w");
        fprintf(fp, "This is an example of output written to file.\n");
        fflush(fp);
        initialise();
        socket_desc = socket(AF_INET , SOCK_STREAM , 0);
        if (socket_desc == -1) {
            fprintf(fp, "Could not create socket\n");
            fflush(fp);
            return -1;
        }
        server.sin_addr.s_addr = inet_addr(ip);
        server.sin_family = AF_INET;
        server.sin_port = htons(port);

        //Connect to remote server
        if (connect(socket_desc , (struct sockaddr *)&server , sizeof(server)) < 0) {
            fprintf(fp, "Connect error\n");
            fflush(fp);
            return -1;
        }
        fprintf(fp, "Connected\n");
        fflush(fp);
        if (socket_desc == -1) {
            return 1;
        }
        while (running == 1) {
            if (firstrun ==1) {
                char tempColour[1];
                if(recv(socket_desc, tempColour , 1, 0) < 0){
                    fprintf(fp,"Receive failed\n");
                    fflush(fp);
                    running = 0;
                    break;
                }
                my_colour = atoi(tempColour);
                MPI_Bcast(&my_colour, 1, MPI_INT, 0, MPI_COMM_WORLD);
                fprintf(fp,"Player colour is: %d\n", my_colour);
                fflush(fp);
                firstrun = 2;
            }

            if(recv(socket_desc, len_buf , 2, 0) < 0) {
                fprintf(fp,"Receive failed\n");
                fflush(fp);
                running = 0;
                break;
            }

            msg_len = atoi(len_buf);

            if (recv(socket_desc, msg_buf, msg_len, 0) < 0) {
                fprintf(fp,"Receive failed\n");
                fflush(fp);
                running = 0;
                break;
            }

            msg_buf[msg_len] = '\0';
            cmd = strtok(msg_buf, " ");

            if (strcmp(cmd, "game_over") == 0) {
                running = 0;
                MPI_Bcast(&running, 1, MPI_INT, 0, MPI_COMM_WORLD);
                fprintf(fp, "Game over\n");
                fflush(fp);
                break;

            } else if (strcmp(cmd, "gen_move") == 0) {
                MPI_Bcast(&running, 1, MPI_INT, 0, MPI_COMM_WORLD);
                my_move = gen_move();
                if (send(socket_desc, my_move, strlen(my_move) , 0) < 0){
                    running = 0;
                    fprintf(fp,"Move send failed\n");
                    fflush(fp);
                    break;
                }
                printboard();
            } else if (strcmp(cmd, "play_move") == 0) {
                opponent_move = strtok(NULL, " ");
                play_move(opponent_move);
                printboard();
            }
            memset(len_buf, 0, 2);
            memset(msg_buf, 0, 15);
        }
        game_over();
    } else {
        run_worker(rank);
        MPI_Finalize();
    }
    return 0;
}

/*
	Called at the start of execution on all ranks
 */
void initialise(){
    int i;
    running = 1;
    globalboard = (int *)malloc(BOARDSIZE * sizeof(int));
    for (i = 0; i<=9; i++) globalboard[i]=OUTER;
    for (i = 10; i<=89; i++) {
        if (i%10 >= 1 && i%10 <= 8) globalboard[i]=EMPTY; else globalboard[i]=OUTER;
    }
    for (i = 90; i<=99; i++) globalboard[i]=OUTER;
    globalboard[44]=WHITE; globalboard[45]=BLACK; globalboard[54]=BLACK; globalboard[55]=WHITE;
}

/*
	Called at the start of execution on all ranks except for rank 0.
	This is where messages are passed between workers to guide the search.
 */
void run_worker(){
    int move, depth, width;
    double start, epsilon;
    score_data scoreData;

    initialise();
    MPI_Bcast(&my_colour, 1, MPI_INT, 0, MPI_COMM_WORLD);
    MPI_Bcast(&running, 1, MPI_INT, 0, MPI_COMM_WORLD);

    depth = 0;
    int terminated = FALSE;

    while (running == 1) {
        MPI_Bcast(globalboard, BOARDSIZE, MPI_INT, 0, MPI_COMM_WORLD);

        start = MPI_Wtime();

        MPI_Status status;
        while(!terminated) {
            MPI_Recv(&move, 1, MPI_INT, 0, 100, MPI_COMM_WORLD, &status);
            scoreData.move = move;
            if (move != -10) {
                width = numlegalmoves(my_colour, globalboard);
                epsilon = (TIMELIMIT/ceil( (float) (width/(size-1))));
                scoreData.score = parallelMinimaxInitialAB(globalboard, move, my_colour, depth, start, epsilon);
                MPI_Send(&scoreData, 1, MPI_2INT, 0, 105, MPI_COMM_WORLD);
            } else {
                break;
            }
        }
        MPI_Bcast(&running, 1, MPI_INT, 0, MPI_COMM_WORLD);
    }
}

/*
	Called when your engine is required to make a move. It must return
	a string of the form "xy", where x and y represent the row and
	column where your piece is placed, respectively.

	play_move will not be called for your own engine's moves, so you
	must apply the move generated here to any relevant data structures
	before returning.
 */
char* gen_move(){
    int i, loc, send, bestmove, bestscore, depth, width, movesReceived, movesSent, *recMoves, *recScores;
    char *move;
    double start;

    if (my_colour == EMPTY){
        my_colour = BLACK;
    }

    MPI_Bcast(globalboard, BOARDSIZE, MPI_INT, 0, MPI_COMM_WORLD);

    //Run in serial if 1 thread otherwise run parallel
    if (size == 1) {
        depth = 0;
        start = MPI_Wtime();
        loc = serialMinimaxInitialAB(globalboard, my_colour, depth, start);
        bestmove = loc;
    } else {
        MPI_Status status;
        movesSent = 0;
        movesReceived = 0;

        //Determine the legal moves to share among other threads
        moves = legalmoves(my_colour, globalboard);
        width = moves[0];

        //If there are no moves to play terminate all threads and send a pass
        if (width <= 0) {
            send = -10;
            for (i = 1; i < size; i++) {
                MPI_Send(&send, 1, MPI_INT, i, 100, MPI_COMM_WORLD);
            }
            return "pass\n";
        }

        recMoves = (int *)malloc(width * sizeof(int));
        recScores = (int *)malloc(width * sizeof(int));

        //Send out moves initially
        for (i = 1; i < size; i++) {
            //Stop sending moves if all have been sent
            if (movesSent == width) {
                break;
            }

            MPI_Send(&moves[i], 1, MPI_INT, i, 100, MPI_COMM_WORLD);
            movesSent++;
        }

        //When a score is sent back give more work if there is work
        int finished = FALSE;
        while (!finished) {
            //Blocks till it receives score data
            score_data scoreData;
            MPI_Recv(&scoreData, 1, MPI_2INT, MPI_ANY_SOURCE, 105, MPI_COMM_WORLD, &status);
            recMoves[movesReceived] = scoreData.move;
            recScores[movesReceived] = scoreData.score;
            movesReceived++;
            
            //When all scores for all moves have been calculated terminate all the loops and exit
            if (movesReceived == width) {
                send = -10;
                for (i = 1; i < size; i++) {
                    MPI_Send(&send, 1, MPI_INT, i, 100, MPI_COMM_WORLD);
                }
                break;
            //Else send another move
            } else if (movesSent < width) {
                send = moves[movesSent+1];
                movesSent++;
                MPI_Send(&send, 1, MPI_INT, status.MPI_SOURCE, 100, MPI_COMM_WORLD);
            }
        }

        //Determine the best move based on the best score
        bestscore = -5000;
        bestmove = recMoves[0];
        for (i = 0; i < movesReceived; i++) {
            if(recScores[i] > bestscore) {
                bestscore = recScores[i];
                bestmove = recMoves[i];
            }
        }
        free(recMoves);
        free(recScores);
    }

    //If there is a valid move make it otherwise pass
    if (bestmove == -1){
        move = "pass\n";
    } else {
        move = get_move_string(bestmove);
        makemove(bestmove, my_colour, globalboard);
    }

    
    free(moves);

    return move;
}

/*
	Called when the other engine has made a move. The move is given in a
	string parameter of the form "xy", where x and y represent the row
	and column where the opponent's piece is placed, respectively.
 */
void play_move(char *move){
    int loc;
    if (my_colour == EMPTY){
        my_colour = WHITE;
    }
    if (strcmp(move, "pass") == 0){
        return;
    }
    loc = get_loc(move);
    makemove(loc, opponent(my_colour), globalboard);
}

/*
	Called when the match is over.
 */
void game_over(){
    MPI_Finalize();
}

char* get_move_string(int loc){
    static char ms[3];
    int row, col, new_loc;
    new_loc = loc - (9 + 2 * (loc / 10));
    row = new_loc / 8;
    col = new_loc % 8;
    ms[0] = row + '0';
    ms[1] = col + '0';
    ms[2] = '\n';
    return ms;
}

int get_loc(char* movestring){
    int row, col;
    row = movestring[0] - '0';
    col = movestring[1] - '0';
    return (10 * (row + 1)) + col + 1;
}

int *legalmoves (int player, int *board) {
    int move, i, *moves;
    moves = (int *)malloc(65 * sizeof(int));
    moves[0] = 0;
    i = 0;
    for (move=11; move<=88; move++)
        if (legalp(move, player, board)) {
            i++;
            moves[i]=move;
        }
    moves[0]=i;
    return moves;
}

int numlegalmoves(int player, int *board) {
    int move, i;
    i = 0;
    for (move=11; move<=88; move++)
        if (legalp(move, player, board)) {
            i++;
        }
    return i;
}

int legalp (int move, int player, int *board) {
    int i;
    if (!validp(move)) return 0;
    if (board[move]==EMPTY) {
        i=0;
        while (i<=7 && !wouldflip(move, ALLDIRECTIONS[i], player, board)) i++;
        if (i==8) return 0; else return 1;
    }
    else return 0;
}

\\ is the play a valid play?
int validp (int move) {
    if ((move >= 11) && (move <= 88) && (move%10 >= 1) && (move%10 <= 8))
        return 1;
    else return 0;
}

int wouldflip (int move, int dir, int player, int *board) {
    int c;
    c = move + dir;
    if (board[c] == opponent(player))
        return findbracketingpiece(c+dir, dir, player, board);
    else return 0;
}

int findbracketingpiece(int square, int dir, int player, int *board) {
    while (board[square] == opponent(player)) square = square + dir;
    if (board[square] == player) return square;
    else return 0;
}

int opponent (int player) {
    switch (player) {
        case 1: return 2;
        case 2: return 1;
        default: printf("illegal player\n"); return 0;
    }
}

int scoreDifference(int *board) {
    return count(my_colour, board);
}

unsigned int zHash(int *board) {
    return 0;
}

int parallelMinimaxInitialAB(int *board, int move, int player, int depth, double start, double timeleft) {
    int val, alpha, beta, *tempBoard;

    alpha = ALPHA;
    beta = BETA;
    tempBoard = (int *)malloc(BOARDSIZE * sizeof(int));

    memcpy(tempBoard, board, BOARDSIZE * sizeof(int));
    makemove(move, player, tempBoard);
    val = minimaxRecurseAB(tempBoard, opponent(player), alpha, beta, depth + 1, start, timeleft);

    free(tempBoard);
    return val;
}

int serialMinimaxInitialAB(int *board, int player, int depth, double start) {
    int i, val, alpha, beta, bestmove, bestscore, width, *moves, *tempBoard;
    double epsilon;

    moves = legalmoves(player, board);
    width = moves[0];
    bestmove = -1;
    bestscore = -5000;
    alpha = ALPHA;
    beta = BETA;
    epsilon = TIMELIMIT;

    if (width == 0) {
        free(moves);
        return -1;
    }

    i = 1;
    tempBoard = (int *)malloc(BOARDSIZE * sizeof(int));
    while (i <= width) {
        memcpy(tempBoard, board, BOARDSIZE * sizeof(int));
        makemove(moves[i], player, tempBoard);
        val = minimaxRecurseAB(tempBoard, opponent(player), alpha, beta, depth + 1, start, epsilon);

        if (val > bestscore) {
            bestscore = val;
            bestmove = moves[i];
        }

        if ((player == my_colour) && (val > alpha)) {
            alpha = val;
        } else if ((player !=  my_colour) && (val < beta)) {
            beta = val;
        }

        if (alpha >= beta) {
            break;
        }

        i++;
    }

    free(moves);
    free(tempBoard);
    return bestmove;

}


int minimaxRecurseAB(int *board, int player, int alpha, int beta, int depth, double start, double timeleft) {
    int i, val, bestscore, width, *moves, *tempBoard;
    double current, elapsed, epsilon;

    current = MPI_Wtime();
    moves = legalmoves(player, board);
    width = moves[0];
    epsilon = timeleft/width;

    if (width == 0) {
        free(moves);
        return scoreDifference(board);
    }

    // Used for the depth first iterative deepening process to determine when it has reached its maximum level
    elapsed = current - start;
    if (epsilon < MINTIME || elapsed >= TIMELIMIT) {
        free(moves);
        return scoreDifference(board);
    }

    i = 1;
    tempBoard = (int *)malloc(BOARDSIZE * sizeof(int));
    while (i <= width) {
        memcpy(tempBoard, board, BOARDSIZE * sizeof(int));
        makemove(moves[i], player, tempBoard);
        val = minimaxRecurseAB(tempBoard, opponent(player), alpha, beta, depth + 1, start, epsilon);

        if (player == my_colour) {
            if (val > alpha) {
                alpha = val;
            }
        } else {
            if (val < beta) {
                beta = val;
            }
        }
        
        // Enacts alpha beta pruning and trims off a branch, reducing runtime
        if (alpha >= beta) {
            break;
        }
        i++;
    }
    
    // Freeing these are integral to space constraints when creating a tree of this size
    free(moves);
    free(tempBoard);

    if (player == my_colour) {
        return alpha;
    } else {
        return beta;
    }

    return bestscore;
}

/*
 * Enacts move onto the board array and calls makeflips to make relevant changes.
 */
void makemove (int move, int player, int *board) {
    int i;
    board[move] = player;
    for (i=0; i<=7; i++) makeflips(move, ALLDIRECTIONS[i], player, board);
}

/*
 * Takes information about a move played and does the corresponding flips on
 * the board
 */
void makeflips (int move, int dir, int player, int *board) {
    int bracketer, c;
    bracketer = wouldflip(move, dir, player, board);
    if (bracketer) {
        c = move + dir;
        do {
            board[c] = player;
            c = c + dir;
        } while (c != bracketer);
    }
}


/*
 * Prints out the board in its current state
 */
void printboard(){
    int row, col;
    fprintf(fp,"   1 2 3 4 5 6 7 8 [%c=%d %c=%d]\n",
            nameof(BLACK), count(BLACK, globalboard), nameof(WHITE), count(WHITE, globalboard));
    for (row=1; row<=8; row++) {
        fprintf(fp,"%d  ", row);
        for (col=1; col<=8; col++)
            fprintf(fp,"%c ", nameof(globalboard[col + (10 * row)]));
        fprintf(fp,"\n");
    }
    fflush(fp);
}

char nameof (int piece) {
    static char piecenames[5] = ".bw?";
    return(piecenames[piece]);
}

/*
 * Returns the current stage of the game when called. This important to know because
 * the stage of the game determines the amount of gravity each factor of the
 * decision making has over the selected move.
 */
int stage() {
    int i, pcoins, ocoins, total, stage;
    pcoins = 0;
    stage = 0;
    ocoins = 0;
    for (i = 1; i <= 88; i++) {
        if (globalboard[i] == my_colour) {
            pcoins++;
        } else if (globalboard[i] == opponent(my_colour)) {
            ocoins++;
        }
    }

    total = pcoins + ocoins;
    if (total <= 20) {
        stage = 1;
    } else if (stage > 20 && stage <= 40) { 
        stage = 2;
    } else if (stage > 40) {
        stage = 3;
    }

    return stage;
}


/*
 * The below positional weighting part of the below evaluation function 
 * is based off the evaluation function in the following repository as 
 * well as the above count function.
 * https://github.com/petersieg/c/blob/master/othello.c
 */
int count (int player, int *board) {
    int i, ocoins, pcoins, ocnt, pcnt, omoves, pmoves, opnt, position, parity, mobility, final;
    
    opnt = opponent(player);
    pcnt = 0;
    pcoins = 0; 
    pmoves = numlegalmoves(player, board);
    ocnt = 0;
    ocoins = 0; 
    omoves = numlegalmoves(opnt, board);

    for (i = 1; i <= 88; i++) {
        if (board[i] == player) {
            pcoins++;
            pcnt = pcnt + WEIGHTS[i];
        } else if (board[i] == opnt) {
            ocoins++;
            ocnt = ocnt + WEIGHTS[i];
        }
    }
    position = (pcnt - ocnt);

    parity = 100 * (pcoins - ocoins)/(pcoins + ocoins);

    if ((pmoves + omoves) > 0) {
        mobility = 100 * (pmoves - omoves)/(pmoves + omoves);
    } else {
        mobility = 0;
    }

    mobility = (3-stage()) * mobility;
    
    final = position + parity + mobility;

    return final;
}

