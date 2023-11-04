#!/bin/bash


# Prompt the user for the number of screens to create

read -p "Enter the number of screens to create: " num_screens


# Create screens and run sudo ./worker

for i in $(seq 1 $num_screens); do

    screen_name="screen$i"

    

    # Create a detached screen with a specified name and execute the command

    screen -S "$screen_name" -dm sudo ./worker

    

    echo "Created screen $screen_name and ran 'sudo ./worker'"

done

