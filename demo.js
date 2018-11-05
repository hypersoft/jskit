#!/usr/bin/jskit

/* global system, parameter */

// include("/some/javascript/file.js")

Shell.setFile('js.out', parameter.join(", ") + '\n');
// Shell.joinFile(FILE, CONTENTS)

var fileContent = Shell.getFile('js.out');
echo("file content:", fileContent);

Shell('rm js.out');

Shell.set('SCRIPT', parameter[0]);
// set(VAR, VALUE, TRUE) // don't overwrite if set

echo("environment variable:", Shell.get('SCRIPT'));
Shell.clear('SCRIPT');
echo(Shell.keys());

if (Shell.get('notavar') !== null) {
	echo("testing environment for empty value failed");
}

Shell.write('cat', Shell.read('ls').output);

echo("you typed: ", Shell.readLine("type something > "));

exit(0);

echo("not reached");
