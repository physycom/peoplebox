#! /usr/bin/env bash

declare -a tags
tags=("20180716-1200" "20180716-1600" "20180716-2000" "20180717-0800" "20180717-1200" "20180719-2000" "20180720-0800")

C0="0.0f"
C1="1.0f"
C2="0.0f"
C3="0.0f"
C4="0.0f"
C5="0.0f"
PROB_IN="0.0f"
PROB_OUT="0.0f"
DARKNET_THRESHOLD="0.5f"
ENABLE_TRIPLET=0
ROTATE_90_COUNTER=0
DETECTION_TYPE_TRACK="tracking"
BARRIER_TOP=0
BARRIER_BOTTOM=1080
BARRIER_LEFT=($(awk 'BEGIN{for(i=700;i<=1220;i+=50)print i}'))
BARRIER_RIGHT=($(awk 'BEGIN{for(i=700;i<=1220;i+=50)print i}'))
TOLL=100
SCALE_X="1.0f"
SCALE_Y="10.0f"
BARRIER_IN="RIGHT"
BARRIER_OUT="LEFT"
PP=0.0625
PM=0.0625
DIRECTION="LEFT_RIGHT"



###############################
## do not touch from here :) ##
###############################

{
  cd "$WORKSPACE/peoplebox/"
  git pull
  git submodule update

  for tag in ${tags[*]}
  do
  for bleft in ${BARRIER_LEFT[*]}
  do
  #for bright in ${BARRIER_RIGHT[*]}
  #do
  bright=${bleft}

  echo "Running with ${tag}_${bleft}_${bright}"

  cd "$WORKSPACE/peoplebox/src/boxvars/"

  INPUTFILE=jetsoncnaf.cmakevars
  rm -f ${INPUTFILE}
  touch ${INPUTFILE}

  {
    echo "set(PEOPLEBOX_ID         \"BETA_CNAF\")"
    echo "set(CAMERA_PROTOCOL      \"/home/nvidia/Downloads/video_${tag}.mp4\")"
    echo "set(CAMERA_IP            \"\")"
    echo "set(CAMERA_CREDENTIALS   \"\")"
    echo "set(CAMERA_FEED_ADDRESS  \"\")"
    echo "set(DARKNET_THRESHOLD    ${DARKNET_THRESHOLD})"
    echo "set(ENABLE_TRIPLET       ${ENABLE_TRIPLET})"
    echo "set(ROTATE_90_COUNTER    ${ROTATE_90_COUNTER})"
    echo "set(DETECTION_TYPE_TRACK \"${DETECTION_TYPE_TRACK}\")"
    echo "set(BARRIER_TOP          ${BARRIER_TOP})"
    echo "set(BARRIER_BOTTOM       ${BARRIER_BOTTOM})"
    echo "set(BARRIER_LEFT         ${bleft})"
    echo "set(BARRIER_RIGHT        ${bright})"
    echo "set(TOLL                 ${TOLL})"
    echo "set(SCALE_X              ${SCALE_X})"
    echo "set(SCALE_Y              ${SCALE_Y})"
    echo "set(BARRIER_IN           \"${BARRIER_IN}\")"
    echo "set(BARRIER_OUT          \"${BARRIER_OUT}\")"
    echo "set(PP                   ${PP})"
    echo "set(PM                   ${PM})"
    echo "set(DIRECTION            \"${DIRECTION}\")"
    echo "set(C0                   ${C0})"
    echo "set(C1                   ${C1})"
    echo "set(C2                   ${C2})"
    echo "set(C3                   ${C3})"
    echo "set(C4                   ${C4})"
    echo "set(C5                   ${C5})"
    echo "set(PROB_IN              ${PROB_IN})"
    echo "set(PROB_OUT             ${PROB_OUT})"
  } >> ${INPUTFILE}

  cd "$WORKSPACE/peoplebox/"
  bash build.sh
  ./darktrack_jetsoncnaf

  cd "$WORKSPACE/peoplebox/data/"
  mkdir -p "$WORKSPACE/peoplebox/data/${tag}_${bleft}_${bright}"
  mv *.json "$WORKSPACE/peoplebox/data/${tag}_${bleft}_${bright}"

  done
  done
  #done

  echo DONE
} &>> log.txt
