#!/usr/bin/jskit

/* global system, parameter */

// include("/some/javascript/file.js")

setFileContent('js.out', parameter.join(", ") + '\n');
// setFileContent.append(FILE, CONTENTS)

var fileContent = getFileContent('js.out');
echo("file content:", fileContent);

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

echo("you typed: ", readLine("type something > "));

exit(0);
