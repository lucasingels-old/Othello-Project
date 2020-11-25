# Algorithmic Othello Player
This is a player which implements the minimax algorithm and slew of improvements 
to make the algorithm faster and make better decisions

## Features

* Minimax algorithm with alpha-beta pruning.
* Parallel calculations of move trees.
* Dynamic Load Balancing of parallel calculations (Parallel Strategy):
    * Accomplished by assigning one move to each thread either till there are no more moves or each thread is occupied
    * Once a thread finishes calculating the tree for the given move it asks for another one
    * If there are no moves left to calculate when a move is requested the thread is asked to terminate itself
* Termination detection of threads when there is no more work for them.
* Iterative deepening
    * Based on a maximum time of 4 seconds per move.
    * The hueristic being the largest part of the calculation was times and this was used as a minimum time slice. Ensuring the deepening would build the tree evenly.
    * An overall timer was built in as a safeguard to make sure that in extenuating circumstances the tree cannot build past the total allocated time of 4 seconds.
* Improved Heuristic Function (AI Strategy):
    * A weighted matrix which aims to influence the evaluation of a move by for instance
	    * Favouring corners.
	    * Not favouring the centre.
	    * Favouring sides except those near the corner for some blocks.
	    * Learning from Othello research, and providing a relative score for each block.
    * Coin parity:
        * The last player to play generally gets to flip a large amount of pieces and it is thus beneficial to assign a score based on yours and the opponents parity.
    * Mobility:
        * Mobility refers to having more moves available to play.
        * We want to increase our mobility and decrease our opponents mobility.
        * The more choices we have the less likely we are to be forced to play bad moves.
        * This works vice versa for the opponent.
        * The game was divided up into stages. 
            * The first stage (<= 20 moves) the mobility score is doubled as this is important in the beginning.
            * The second stage (> 20 moves and <= 40 moves) the score is multiplied by 1 as it is less important at this point but still important.
            * The third stage (> 40 moves) the score is multiplied by 0 to give more emphasis to parity and position.
* When given only one thread the program will run a serial version of the minimax alpha-beta algorithm up to the maximum depth time will allow.

## Versioning

This is the first edition. Later editions may include the ability to beat a grandmaster.
This version sees Logistello as its role model and aspires to be like it one day. 
If this code could speak it would say "humanity needs a lesson in humility and I hope to provide that one day"

## Compiling and Running

Before you compile, make sure you have mpicc installed. You can do this with HomeBrew on MacOS by running:

```
brew install open-mpi
```

or on Ubuntu by running:

```
sudo apt-get install -y mpi
```

Open and edit the make file if running on Ubuntu. The ubuntu terminal code is present but commented out. Use the provided makefile to compile the program into an executable and the included run.sh file to run the player against itself. 

```
make all
./run.sh
```

If you wish to run this player against another compatible player you can add the relative path to your player in the game.json file
and leave the other player as player/parminimaxab

## Authors

* **Luca Singels** - *All Work* - https://git.cs.sun.ac.za/20745249)

## References
The weighted matrix is based off the following paper:
* Szubert, Marcin & Jaśkowski, Wojciech & Krawiec, Krzysztof. (2011). Learning Board Evaluation Function for Othello by Hybridizing Coevolution with Temporal Difference Learning. Control and Cybernetics. 40. 805-831.
 
The evaluation function named **count** is based off the evaluation function in the following repository:

* https://github.com/petersieg/c/blob/master/othello.c

The Ingenious Framework which is the engine behind the game is provided by the Computer Science Department of Stellenbosh University
