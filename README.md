# ssstatus
ssstatus (**s**uper **s**imple status) is a bloat-free status line generator for i3bar. It minimises CPU time and is customisable with its own simple configuration format. ssstatus does nothing more and nothing less than it needs to do, it is currently standing at ~150 lines of C99 code.

## Compiling
ssstatus doesn't have any dependencies, you can trivially compile it with any compiler conforming to C99, POSIX.1, and the 4.4BSD `strsep` function

## Usage
ssstatus reads its configuration from the standard input. You can use it as any other status generator for i3bar, simply specify the invoking command in the status_command option of the bar block in your i3 configuration file.
```
bar {
    status_command ssstatus < ~/.ssstatus
}
```
```
status_command echo "date +%T;1s;;" | ssstatus
```

## Configuration
ssstatus can be configured using its its minimal configuration format.
This format is designed to be easy to read, parse and not rely on newline characters.  
Each status entry is made up of fields separated by semicolons (`;`) and is terminated by double semicolons (`;;`).

The first field is the shell command whose first line of the standard output is to be displayed in the status. 

The second field specifies either the update interval of the entry or the POSIX real-time signal only upon which the entry be updated. This entry is made up of a base-10 integer immediately followed by `s`(seconds), `m`(minutes), `h`(hours), `!`(The real-time signal index, the actual signal number will be `SIGRTMIN`+ the specified index).

The third field is optional and specifies the text colour of the entry as a hexadecimal integer.

Comments can be inserted by enclosing them with `#` on both sides. Comments can only appear in between entries.

Follows an example of a configuration file:
```
# Date and time, every 5 seconds, default colour #
date +"%D %T"; 5s;;
# Current weather, hourly, purple #
curl -Ss wttr.in?format="%C+%f"; 1h; 0xD0D0FF;;
# Current keyboard layout, upon SIGRTMIN+8, green 
  `pkill -$((65+8)) ssstatus` can be used to update on FreeBSD#
setxkbmap -query | awk '/layout/{print $2}'; 8!; d0ffd0;;
```
The following status is produced:
![A screenshot of the resulting status bar](screen1.png)

## Errata
- The configuration parser assumes valid input and otherwise has undefined behaviour. Unless the supplied configuration is crafted with malicious intent, this will usually only crash ssstatus.
- Command output is cut off at the first line break and echoed verbatim, this can result in invalid JSON output. (E.g. when the command output contains unescaped quotation marks (").)
- All commands are executed on a single thread in blocking mode, the timing requirements are only realised on a best-effort basis and each command cannot queue up more than once.
