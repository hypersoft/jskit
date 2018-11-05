#!/usr/bin/smash --shell-script

/* global system, parameter */

setFileContent('js.out', parameter.join(", ") + '\n');

var output = getFileContent('js.out');
echo("file contents:", output);
system('rm js.out');

set('SCRIPT', parameter[0]);
echo("environment variable:", get('SCRIPT'));
clear('SCRIPT');
echo(keys());

if (get('notavar') !== null) {
	echo("testing environment for empty value failed");
}

system.write('cat', system.read('ls').output);

echo("you typed: ", readLine("type something > "));
exit(0);
