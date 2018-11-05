#!/usr/bin/jskit

/* global system, parameter */

// include("/some/javascript/file.js")

setFileContent('js.out', parameter.join(", ") + '\n');
// setFileContent.append(FILE, CONTENTS)

var output = getFileContent('js.out');
echo("file contents:", output);
system('rm js.out');

set('SCRIPT', parameter[0]);
// set(VAR, VALUE, TRUE) // don't overwrite if set

echo("environment variable:", get('SCRIPT'));
clear('SCRIPT');
echo(keys());

if (get('notavar') !== null) {
	echo("testing environment for empty value failed");
}

system.write('cat', system.read('ls').output);

// this doesn't exist yet
echo("you typed: ", readLine("type something > "));

// this doesn't exist yet
exit(0);
