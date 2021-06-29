#!/bin/sh

USAGE="Usage: `basename $0` [-h] [-t embuitag] args"

# etag file
tags=etags.txt
# embui branch/tag name to fetch
embuitag="dev"

refresh_styles=0


# parse cmd options
while getopts ht: OPT; do
    case "$OPT" in
        h)
            echo $USAGE
            exit 0
            ;;
        t)
            echo "EmbUI tag is set to: $OPTARG"
            embuitag=$OPTARG
            ;;
        \?)
            # getopts issues an error message
            echo $USAGE >&2
            exit 1
            ;;
    esac
done



[ -f $tags ] || touch $tags

# check github file for a new hash
freshtag(){
    local url="$1"
    etag=$(curl -sL -I $url | grep etag | awk '{print $2}')
    echo "$url $etag" >> newetags.txt
    if [ $(grep -cs $etag $tags) -eq 0 ] ; then
        #echo "new tag found for $url"
        return 0
    fi
    #echo "old tag for $url"
    return 1
}


echo "Preparing resources for FireLamp FS image into ../data/ dir" 

mkdir -p ../data/css ../data/js

# собираем скрипты и стили из репозитория EmbUI (используется dev ветка, при формировании выпусков пути нужно изменить)
if freshtag https://github.com/DmytroKorniienko/EmbUI/raw/$embuitag/resources/data.zip ; then
    refresh_styles=1
    echo "EmbUI resources requires updating"
else
    echo "EmbUI is up to date"
fi


#    rm -f embui.zip

# update static files if newer
[ ! -f ../data/index.html.gz ]  || [ html/index.html -nt ../data/index.html.gz ] && gzip -9k html/index.html && mv -f html/index.html.gz ../data/
[ ! -f ../data/favicon.ico.gz ] || [ html/favicon.ico -nt ../data/favicon.ico.gz ] &&  gzip -9k html/favicon.ico && mv -f html/favicon.ico.gz ../data/


mv -f newetags.txt $tags

#cd ./data
#zip --filesync -r -0 --move --quiet ../data.zip .
#cd ..
#rm -r ./data
#echo "Content of data.zip file should be used to create LittleFS image"