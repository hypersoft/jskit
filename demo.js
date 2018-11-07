#!/bin/spider --shell-script

/**
 * function Number Shell(String command);
 * 
 * Operation: Execute System Command, returning status integer,
 *            where 0 is success and 1 or more is an error. Standard
 *            I/O for the command is inherited from the current process.
 *  test:
 */ if (Shell("echo Hello Worldian!") !== 0) {
		Shell.error("something went wrong calling the system echo command");
		exit(1);
 } 
//

/**
 * function Object Shell.readPipe(String command);
 * 
 * Operation: Execute System Command, returning an object representing status,
 *            and output.
 * 
 *  test:
 */ var capture = Shell.readPipe("echo gotcha!");
 if (capture.output !== 'gotcha!\n' || capture.status !== 0) {
	 Shell.error("something went wrong executing read command pipe");
	 exit(1);
 }

 /**
	* function Number Shell.writePipe(String command, String input);
	*
	* Operation: Execute System Command using input for the command's
	*            standard input handle, and return system execution
	*            status flag.
	*
	*  test:
  */if (Shell.writePipe("grep Javascript", "Javascript Lessons!\nJava Lessons") !== 0) {
		Shell.error("something went wrong while writing to command pipe");
		exit(1);
	}

	// That's it for execution functions. Try your hands on: Environment functions

	/**
	 * function String[] keys();
	 * 
	 * Operation: Returns an array of all defined environment variable keys.
	 * 
	 *  test:
	 */ var eKeys = Shell.keys();
	 if (! eKeys) {
		 Shell.error("something went wrong obtaining the environment keys");
	 } else {
		 Shell.echo("You have", eKeys.length, "variables defined within your current environment.");
	 }
	 //

/**
 * function String Shell.get(String key); 
 * 
 * Operation: Retrievies the value of the variable for the given key
 *            from the environment.
 *  test:
 */ if (typeof Shell.get('USER') !== 'string') {
	 Shell.error("couldn't get USER key from environment");
	 exit(1);
 } else echo("And your user name seems to be:", Shell.get('USER'));

/**
 * function Boolean Shell.set(String key, String value[, Boolean default]);
 * 
 * Operation: Sets the given key to the given value within the current
 *            environment. If default is true, then the value will not
 *            be overwritten if it already exists.
 *  test:
 */ Shell.set('GHOST', undefined);
 if (Shell.get('GHOST') !== 'undefined') {
	 Shell.echo("something went wrong setting the environment variable: GHOST");
	 exit(1);
 }
 Shell.set('GHOST', 'Javascript', true);
 if (Shell.get('GHOST') !== 'undefined') {
	 Shell.echo("something went wrong while testing protected environment variable: GHOST");
	 exit(1);
 }

 /// that's all for now...
exit();
var x = Shell.buffer(8, 2);
x.double = true;
x[0] = 10.1;
x[1] = 10.2;
print(x[0], " = *((double *)", x.name, ")", '\n');

var bytesWritten = Shell.writeFile('js.out', parameter.join(", ") + '\n');

// Shell.source("/some/javascript/file.js")
var chars = Shell.buffer(1, bytesWritten);
chars.utf = true;
echo((Shell.fd.type(Shell.fd[0])));
var fileptr = Shell.fd.openFile('js.out', 'r');
Shell.fd.read(fileptr, chars, bytesWritten);
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
