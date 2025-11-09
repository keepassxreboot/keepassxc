#!/usr/bin/env python3
from collections import defaultdict
import json
import sys
from pathlib import Path
from urllib import request

txrc = Path.home() / '.transifexrc'
if not txrc.exists():
    print('No Transifex config found. Run tx init first.')
    sys.exit(1)

org = 'o:keepassxc'
proj = f'{org}:p:keepassxc'
resource = f'{proj}:r:share-translations-keepassxc-en-ts--master'
token = [l for l in open(txrc, 'r') if l.startswith('token')][0].split('=', 1)[1].strip()
member_blacklist = ['u:droidmonkey', 'u:phoerious']


def get_url(url):
    req = request.Request(url)
    req.add_header('Content-Type', 'application/vnd.api+json')
    req.add_header('Authorization', f'Bearer {token}')
    with request.urlopen(req) as resp:
        return json.load(resp)


print('Fetching languages...', file=sys.stderr)
languages_json = get_url(f'https://rest.api.transifex.com/projects/{proj}/languages')
languages = {}
for lang in languages_json['data']:
    languages[lang['id']] = lang['attributes']['name']

print('Fetching language stats...', file=sys.stderr)
language_stats_json = get_url('https://rest.api.transifex.com/resource_language_stats?'
                              f'filter[project]={proj}&filter[resource]={resource}')
completion = {}
for stat in language_stats_json['data']:
    completion = stat['attributes']['translated_strings'] / stat['attributes']['total_strings']
    if completion < .6:
        languages.pop(stat['relationships']['language']['data']['id'])

print('Fetching language members...', end='', file=sys.stderr)
members_json = get_url(f'https://rest.api.transifex.com/team_memberships?filter[organization]={org}')
members = defaultdict(set)
for member in members_json['data']:
    print('.', end='', file=sys.stderr)
    if member['relationships']['user']['data']['id'] in member_blacklist:
        continue
    lid = member['relationships']['language']['data']['id']
    if lid not in languages:
        continue
    user = get_url(member['relationships']['user']['links']['related'])['data']['attributes']['username']
    members[lid].add(user)
print(file=sys.stderr)

print('<ul>')
for lang in sorted(languages, key=lambda x: languages[x]):
    if not members[lang]:
        continue
    lines = [f'    <li><strong>{languages[lang]}:</strong> ']
    for i, m in enumerate(sorted(members[lang], key=lambda x: x.lower())):
        if len(lines[-1]) + len(m) >= 120:
            lines.append('        ')
        lines[-1] += m
        if i < len(members[lang]) - 1:
            lines[-1] += ', '
    lines[-1] += '</li>'
    print('\n'.join(lines))
print('</ul>')
