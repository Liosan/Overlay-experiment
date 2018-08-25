A proof-of-concept implementation of an in-game overlay. Based on MS Detours.

Basis of operation:
- locate The Witcher 1 process
- inject our dll into the game process and launch a thread running code from that dll
- hook DX9 rendering functions by creating a trampoline allowing us to intercept the call, and render our own textures
- similarly hook input

Done as a homework assignment from a recruitment process. Tested on The Witcher 1.

All commits are mine. The git history losed usernames when migrating from BitBucket: https://bitbucket.org/Liosan/overlay