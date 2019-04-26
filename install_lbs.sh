#!/bin/bash
#DISCLAIMER!!!
#This script is intended to be executed once and only once, altering your Charm++
# Makefile and Makefile_lb.sh files.
# It could add unecessary dependencies to your Charm installation if ran more than once

#Dont forget to redefine your CHARM_DIR
CHARM_DIR=~/charm
TMP_DIR=$CHARM_DIR/tmp
WORK_DIR=$(pwd)
LB_LIST="PackDropLB PackStealLB"
INC_LIST="UpdateProcMap.h UpdateWorkMap.h"
DEPS_LIST="UpdateProcMap.o UpdateWorkMap.o"

# This copies the files from pwd to the charm tmp dir:
cp $WORK_DIR/schemes/Update* $TMP_DIR/ # Copies the dependencies
cp $WORK_DIR/charm/algorithms/* $TMP_DIR/ # Copies the LB algorithms


# Update the Charm++ Makefile and Makefile_lb.sh
echo "Walking to Charm dir: "
cd; cd $TMP_DIR # Go to charm dir
echo "I am at: $(pwd)"

# This should add our LBs to the Makefile_lb.sh file:
awk -v lbs="$LB_LIST" '{gsub("COMMON_LBS=\"","COMMON_LBS=\""lbs" "); print}' Makefile_lb.sh > Makefile_lb.sh
echo "#This file has been modified by Packing Schemes installer" >> Makefile_lb.sh

# This should add our dependencies to the Charm++ Makefile, we add ourselves to the Chare Kernel dependencies here:
awk -v deps="$DEPS_LIST" '{gsub("ckgraph.o","ckgraph.o "deps); print}' Makefile > Makefile
echo "Done adding dependencies ..."
awk -v includes="$INC_LIST" '{gsub("ckgraph.h","ckgraph.h "includes); print}' Makefile > Makefile
echo "Done adding core headers ..."

echo "Recompiling Charm++ with new LBs"
./Makefile_lb
make depends -j; make charm++ -j2
echo "Done"

cd $WORK_DIR
