#! /usr/bin/bash
# prvni projekt do ios 
# vytvoril: Jakub Lukas aka xlukas18

FILENAME=$0	# name of source file
ARGC=0
ARGCC=0
COMMAND="nott"
DATEB="2022-03-27"
DATEA="2020-03-01"
GENDER=""
ARGFC=0
WIDTH=0

sortInput()
{	
	if [[ $1 == "-a" ]]
	then
		DATEA=$2

	elif [[ $1 == "-b" ]]
	then
		DATEB=$2
	
	elif [[ $1 == "-g" ]]
	then
		if [[ $2 == "M" || $2 == "Z" ]]
		then
			GENDER=$2
		else
			echo "invalid filter data"
			exit
		fi
	
	elif [[ $1 == "-s" ]]
	then
		WIDTH=$2
	fi
	
       # checks command on input argument
        for i in $@
        do
		if [[ $i =~ (.(.cvs)) ]]
		then
			FILENAME=$i
		fi

                for j in infected merge gender age daily monthly yearly countries regions
                do
                        if [[ $i == $j ]]
                        then
                                COMMAND=$i
                                ARGCC=$((ARGCC+1))      #FIXME
                        fi
                done
        done

	# checks if there is not more then 1 command    
        if [ $ARGC > 1 ]
        then
                echo "input error: invalid number of commands"
        fi

}

cntGender()
{
	echo "muzi: "; grep -c ",M," $FILENAME
	echo "zeny: "; grep -c ",Z," $FILENAME
}

cntInfected()
{
	grep -c "," $FILENAME
}	

sortInput $@
echo "command:" $COMMAND
echo "number of commands: " $ARGCC
echo "gender: "$GENDER
echo "width: "$WIDTH
echo "filename: "$FILENAME

if [[ "$COMMAND" == "gender" ]];
then
	cntGender
elif [[ "$COMMAND" == "infected" ]]
then
	cntInfected
fi

# echo "Script Name: $0"
# echo "First Parameter of the script is $1"
# echo "The second Parameter is $2"
# echo "The complete list of arguments is $@"
# echo "Total Number of Parameters: $#"
# echo "The process ID is $$"
# echo "Exit code for the script: $?"

