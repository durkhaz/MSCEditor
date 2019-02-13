# MSCEditor

	MSCeditor - a save editor			by durkhaz
	For the (great) game "My Summer Car"		by Amistech Games 

**Q: What does it do?**

It's a parser for the Unity Plugin "EasySave 2", allowing you to edit your saves as you please!
Complete set of features:
 - A World map rendered with Direct2D (Requires Windows 8.1 or newer)
 - Add and remove entries
 - Teleport objects
 - Identify problems with your car
 - Spawn items
 - Modify time and weather

**Q: Where do I find my saves?**

AppData\LocalLow\Amistech\My Summer Car\. The tool should open the correct folder on first launch though.

**Q: There's 6 files, which one do I open?**

 Most entries of interest are inside "defaultES2File". But you could open the other ones too.

**Q: I changed some values and now the game won't load my savegame Is this a bug?**

No, I tried my best to validate all the data, but there's only so much I can do. If you want to break the game, there's always a way.

**Q: Why do entries show up red?**

red = unsaved modified entries. Right click to reset them to their file state

**Q: How do backups work?**

When the option is enabled, it saves backups to the same folder the savegames are in.  It will make a backup everytime you save, so no data can be lost!

**Q: What data-types are currently supported?**

Transform, Float, String, Boolean, Color, Integer, Vector3
Containers: NativeArray, Dictionary, List, HashSet, Queue, Stack
