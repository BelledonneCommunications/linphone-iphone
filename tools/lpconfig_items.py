#!/usr/bin/python

from collections import defaultdict
items = defaultdict(list)

def get_files_in_coreapi_directory():
	from os import walk
	files = []
	for (dirpath, dirnames, filenames) in walk('../coreapi'):
		files.extend(filenames)
		break
	return files

def parse_file(filename):
	with open('../coreapi/' + filename, 'r') as infile:
		for line in infile:
			if 'lp_config_get_' in line:
				parse_lpconfig_line(line)

def parse_lpconfig_line(line):
	token = line[line.find('lp_config_get_') + len('lp_config_get_'):]
	split = token.split('(', 1)
	item_type = split[0]
	if '_' in item_type:
		return
	
	params_split = split[1].split(',', 3)
	item_section = params_split[1]
	if item_section[0] != '"':
		return
	item_section = item_section.split('"')[1]
	
	item_name = params_split[2]
	if item_name[0] != '"':
		return
	item_name = item_name.split('"')[1]
	
	item_default_value = params_split[3].split(')')[0]
	if item_type == 'string' and item_default_value[0] != '"':
		item_default_value = '<unknown_string>'
	
	item = [item_type, item_name, item_default_value]
	items[item_section].append(item)

for files in get_files_in_coreapi_directory():
	parse_file(files)
for section, items in items.iteritems():
	print '[' + section + ']'
	for item in items:
		print item[1] + '=' + item[2]
	print ''