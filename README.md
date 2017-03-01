# MSCEditor

	MSCeditor - a savegame editor				 	by durkhaz
	For the (great) game "My Summer Car"		by Amistech Games 

Q: What does it do?

A: It converts binary data to human readable form and back, allowing you to edit your savegames as you please!
It currently supports the following datatypes: FLOAT, BOOL, ARRAY, INT, STRING, VECTOR3, VECTOR4.  

Q: Where do I find my savegames?

A: AppData\LocalLow\Amistech\My Summer Car\. The tool should open the correct folder on first launch though.

Q: There's 6 files, which one do I open?

A: The entries of interest are inside "defaultES2File". But you could open the other ones too.

Q: I changed some values and now the game won't load my savegame Is this a bug?

A: No, I tried my best to validate all the data, but there's only so much I can do. If you want to break the game, there's always a way.

Q: Why do entries show up red?

A: red = unsaved modified entries. Right click to reset them to their file state

Q: How do backups work?

A: When the option is enabled, it saves backups to the same folder the savegames are in.  It will make a backup everytime you save, so no data can be lost!
