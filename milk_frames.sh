#!/bin/bash

if [[ $1 == "" ]]; then
  echo "Usage : $0 [boxid|all|active]"
fi

source "../venice_fs/usr/bin/peoplebox-init"
source "../venice_fs/.ssh/ip.sh" $1

for indice in ${!nomi[*]}
do
  nome="${nomi[$indice]}"
  indirizzo="${indirizzi[$indice]}"
  if [[ $nome = *"raspi"* ]]; then
    nomeuser=pi
    password=raspberry
  else
    nomeuser=nvidia
    password=nvidia
  fi
  echo "Milking frames from $nomeuser@$nome"

  for i in {1..1}; do
    # sync dump_frame and setup
    echo scp dump_frame "${nomeuser}"@"${nome}":"$WORKSPACE"
    echo ssh "${nomeuser}"@"${nome}" mkdir -p "$WORKSPACE"frames

    # launch dump_frame with proper command line
    echo ssh "${nomeuser}"@"${nome}" "$WORKSPACE"/dump_frame 2 rtsp://physycom:ohnounapassWORD55@${camip}:554/rtpstream/config1=u

    # rsync output
    frames=$(find "$WORKSPACE"/"frames" -type f -name "*.jpg")
    if [[ $frames != "" ]]; then
      echo $RSYNC --remove-source-files "${RSYNC_FLAGS}" -e "${server_SSH}" "$frames" "$server_user"@"$server_host":"$JSON_SYNC_PATH"
    fi
  done
done
