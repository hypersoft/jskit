#!/bin/spider --shell-script

/* global function Shell(cmd), parameter */

var x = Shell.buffer(8, 2);
x.double = true;
x[0] = 10.1;
x[1] = 10.2;
print(x[0], " = *((double *)", x.name, ")", '\n');

var bytesWritten = Shell.writeFile('js.out', parameter.join(", ") + '\n');

// Shell.source("/some/javascript/file.js")
var chars = Shell.buffer(1, bytesWritten);
chars.utf = true;
var fileptr = Shell.fd.openFile('js.out', 'r');
Shell.fd.read(fileptr, chars);
Shell.fd.close(fileptr);
fileptr = Shell.fd.openFile('js.out', 'r');
var chars2 = new Array(bytesWritten);
Shell.fd.read(fileptr, chars2)
chars.utf = false;
echo(chars2[0] === chars[0])
Shell.fd.write(Shell.fd[1], chars, chars.length);
echo();
echo(Shell.buffer.slice(chars));
echo(chars2)

//echo(Shell.buffer.slice(chars));

// Shell.joinFile(FILE, CONTENTS) // for append mode

var fileContent = Shell.readFile('js.out');
echo("file content:", fileContent);

Shell('rm js.out');

Shell.set('SCRIPT', parameter[0]);
// set(VAR, VALUE, TRUE) // don't overwrite if set

echo("environment variable:", Shell.get('SCRIPT'));
Shell.clear('SCRIPT');

// list all environment variables
//echo(Shell.keys());

if (Shell.get('notavar') !== null) {
	echo("testing environment for empty value failed");
}

Shell.writePipe('cat', Shell.readPipe('ls').output);

//echo("you typed: ", Shell.readLine("type something > "));

echo("All properties of Function: Shell:", Object.keys(Shell).join(", "));
echo("exiting...");

exit();

echo("not reached");
