#************************************************************************************************************************************
#*	game.json the engine configuration file
#*	It has 4 possible options.
#*	numPlayers ==> this must be 2
#*	threads ==> this is the number of threads to run your player with; the default is 1
#*	time ==> this is the time a player has to make a move, unit is seconds, doubles are allowed.
#*	path1 ==> this is the path to your first C player, note that this needs to be in a folder, path cannot be just the player name.
#*	path2 ==> this is the path to your second C player, note that this needs to be in a folder, path cannot be just the player name.
#*
#* 	
#************************************************************************************************************************************
if [ -z "$1" ] || [ -z "$2" ]|| [ -z "$3" ]|| [ -z "$4" ]; then

echo "Usage: ./run.sh player1 player2 int_val_time_out_in_seconds num_processes"
#exit
else
echo "{
	\"numPlayers\": 2,
	\"threads\": $4,
	\"time\": $3,
	\"path1\": \"$1\",
	\"path2\": \"$2\"
}" > game.json
fi
#Stops gameserver if it is already running
fuser -n tcp -k 61234 
#kill $(lsof -t -i:61234)


#Starts the server in a new terminal
#gnome-terminal -x sh -c "java -jar IngeniousFramework.jar server; bash"
#for macOS
xterm -e "java -jar IngeniousFramework.jar server; bash" &
sleep 2
#Starts the othello lobby
java -jar IngeniousFramework.jar create -config "game.json" -game "OthelloReferee" -lobby "mylobby"

#runs a player name foo
#gnome-terminal -x sh -c "java -jar IngeniousFramework.jar client -username foo -engine za.ac.sun.cs.ingenious.games.othello.engines.OthelloMPIEngine -game OthelloReferee -hostname localhost -port 61234; bash"
#for macOS
xterm -e "java -jar IngeniousFramework.jar client -username foo -engine za.ac.sun.cs.ingenious.games.othello.engines.OthelloMPIEngine -game OthelloReferee -hostname localhost -port 61234; bash" &

#This will pipe to file instead
#gnome-terminal -x sh -c "java -jar IngeniousFramework.jar client -username foo -engine za.ac.sun.cs.ingenious.games.othello.engines.OthelloMPIEngine -game OthelloReferee -hostname localhost -port 61234 > foo"

#runs a player name foo
#gnome-terminal -x sh -c "java -jar IngeniousFramework.jar client -username bar -engine za.ac.sun.cs.ingenious.games.othello.engines.OthelloMPIEngine -game OthelloReferee -hostname localhost -port 61234; bash"
#for macOS
xterm -e "java -jar IngeniousFramework.jar client -username bar -engine za.ac.sun.cs.ingenious.games.othello.engines.OthelloMPIEngine -game OthelloReferee -hostname localhost -port 61234; bash" &


#This will pipe to file instead
#gnome-terminal -x sh -c "java -jar IngeniousFramework.jar client -username bar -engine za.ac.sun.cs.ingenious.games.othello.engines.OthelloMPIEngine -game OthelloReferee -hostname localhost -port 61234 > bar"


