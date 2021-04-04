#!/bin/sh

yesorno()
{
	echo "Is your name $* ?"
	while true
		do
			echo -n "enter yes or no :"
			read x
			case "$x" in
				[yY] | [yY][eE][sS] )  return 0;;
				[nN] | [nN][oO] )		return 1;;
				* )					echo "answer yes or no"
			esac
		done
}

echo "original parameters are $* "

if yesorno "$1"
then echo "Hi $1,nice name"
else echo "Never mind"
fi
exit 0
