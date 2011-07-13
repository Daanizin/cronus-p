// Copyright (c) Athena Dev Teams - Licensed under GNU GPL
// For more information, see LICENCE in the main folder

#include "../common/mmo.h"
#include "../common/showmsg.h"
#include "../common/malloc.h"
#include "../common/strlib.h"
#include "mapindex.h"

#include <string.h>
#include <stdio.h>
#include <stdlib.h>

struct _indexes {
	char name[MAP_NAME_LENGTH]; //Stores map name
} indexes[MAX_MAPINDEX];

int max_index = 0;

char mapindex_cfgfile[80] = "db/map_index.txt";

#define mapindex_exists(id) (indexes[id].name[0] != '\0')

/// Retrieves the map name from 'string' (removing .gat extension if present).
/// Result gets placed either into 'buf' or in a static local buffer.
const char* mapindex_getmapname(const char* string, char* output)
{
	static char buf[MAP_NAME_LENGTH];
	char* dest = (output != NULL) ? output : buf;
	
	size_t len = strnlen(string, MAP_NAME_LENGTH_EXT);
	if (len == MAP_NAME_LENGTH_EXT) {
		ShowWarning("(mapindex_normalize_name) Nome do mapa '%*s' � muito grande!\n", 2*MAP_NAME_LENGTH_EXT, string);
		len--;
	}
	if (len >= 4 && stricmp(&string[len-4], ".gat") == 0)
		len -= 4; // strip .gat extension
	
	len = min(len, MAP_NAME_LENGTH-1);
	strncpy(dest, string, len+1);
	memset(&dest[len], '\0', MAP_NAME_LENGTH-len);
	
	return dest;
}

/// Retrieves the map name from 'string' (adding .gat extension if not already present).
/// Result gets placed either into 'buf' or in a static local buffer.
const char* mapindex_getmapname_ext(const char* string, char* output)
{
	static char buf[MAP_NAME_LENGTH_EXT];
	char* dest = (output != NULL) ? output : buf;

	size_t len;

	strcpy(buf,string);
	sscanf(string,"%*[^#]%*[#]%s",buf);

	len = safestrnlen(buf, MAP_NAME_LENGTH);

	if (len == MAP_NAME_LENGTH) {
		ShowWarning("(mapindex_normalize_name) Nome do mapa '%*s' � muito grande!\n", 2*MAP_NAME_LENGTH, buf);
		len--;
	}
	strncpy(dest, buf, len+1);

	if (len < 4 || stricmp(&dest[len-4], ".gat") != 0) {
		strcpy(&dest[len], ".gat");
		len += 4; // add .gat extension
	}

	memset(&dest[len], '\0', MAP_NAME_LENGTH_EXT-len);
	
	return dest;
}

/// Adds a map to the specified index
/// Returns 1 if successful, 0 oherwise
int mapindex_addmap(int index, const char* name)
{
	char map_name[MAP_NAME_LENGTH];

	if (index == -1){
		for (index = 1; index < max_index; index++)
		{
			//if (strcmp(indexes[index].name,"#CLEARED#")==0)
			if (indexes[index].name[0] == '\0')
				break;
		}
	}

	if (index < 0 || index >= MAX_MAPINDEX) {
		ShowError("(mapindex_add) Map index (%d) para \"%s\" ultrapassou o limite (max is %d)\n", index, name, MAX_MAPINDEX);
		return 0;
	}

	mapindex_getmapname(name, map_name);

	if (map_name[0] == '\0') {
		ShowError("(mapindex_add) N�o � poss�vel adicionar mapas sem nome.\n");
		return 0;
	}

	if (strlen(map_name) >= MAP_NAME_LENGTH) {
		ShowError("(mapindex_add) Nome do mapa %s � muito grande. O lim�te m�ximo � de %d carateres.\n", map_name, MAP_NAME_LENGTH);
		return 0;
	}

	if (mapindex_exists(index))
		ShowWarning("(mapindex_add) Apagando index %d: map \"%s\" -> \"%s\"\n", index, indexes[index].name, map_name);

	safestrncpy(indexes[index].name, map_name, MAP_NAME_LENGTH);
	if (max_index <= index)
		max_index = index+1;

	return index;
}

unsigned short mapindex_name2id(const char* name)
{
	//TODO: Perhaps use a db to speed this up? [Skotlex]
	int i;

	char map_name[MAP_NAME_LENGTH];
	mapindex_getmapname(name, map_name);

	for (i = 1; i < max_index; i++)
	{
		if (strcmp(indexes[i].name,map_name)==0)
			return i;
	}
	ShowDebug("mapindex_name2id: Mapa \"%s\" n�o foi encontrado no index!\n", map_name);
	return 0;
}

const char* mapindex_id2name(unsigned short id)
{
	if (id > MAX_MAPINDEX || !mapindex_exists(id)) {
		ShowDebug("mapindex_id2name: Nome de mapa indexistente requisitado [%d] no cache.\n", id);
		return indexes[0].name; // dummy empty string so that the callee doesn't crash
	}
	return indexes[id].name;
}

void mapindex_init(void)
{
	FILE *fp;
	char line[1024];
	int last_index = -1;
	int index;
	char map_name[1024];
	
	memset (&indexes, 0, sizeof (indexes));
	fp=fopen(mapindex_cfgfile,"r");
	if(fp==NULL){
		ShowFatalError("N�o foi poss�vel ler a configura��o: %s!\n", mapindex_cfgfile);
		exit(EXIT_FAILURE); //Server can't really run without this file.
	}
	while(fgets(line, sizeof(line), fp))
	{
		if(line[0] == '/' && line[1] == '/')
			continue;

		switch (sscanf(line, "%1023s\t%d", map_name, &index))
		{
			case 1: //Map with no ID given, auto-assign
				index = last_index+1;
			case 2: //Map with ID given
				mapindex_addmap(index,map_name);
				break;
			default:
				continue;
		}
		last_index = index;
	}
	fclose(fp);
}

int mapindex_removemap(int index){
	indexes[index].name[0] = '\0';
	return 0;
}

void mapindex_final(void)
{
}
