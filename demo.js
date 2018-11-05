#!/usr/bin/jskit --shell-script

/* global function Shell(cmd), parameter */

// Shell.source("/some/javascript/file.js")

Shell.writeFile('js.out', parameter.join(", ") + '\n');
// Shell.joinFile(FILE, CONTENTS) // for append mode

var fileContent = Shell.readFile('js.out');
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

Shell.writePipe('cat', Shell.readPipe('ls').output);

echo("you typed: ", Shell.readLine("type something > "));

echo("All properties of Function: Shell:", Object.keys(Shell).join(", "));
echo("exiting...");
exit(0);

echo("not reached");
