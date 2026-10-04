Railgun mod for The Last Caretaker

Copy Railgun.utoc, Railgun.ucas and Railgun.pak to
Voyage\Content\Paks. Place Railgun.ini in the same directory. Copy
Mods\RailgunCatalogue\RailgunCatalogue.uplugin to
Voyage\Mods\RailgunCatalogue\RailgunCatalogue.uplugin. Keep the descriptor and
all three Railgun files from the same build together. Before updating from a
legacy build, retire its Railgun.autoload through the reviewed recoverable
migration; leaving it installed will request a class that no longer exists. Do
not extract the whole ZIP into Voyage\Content\Paks; it contains files for two
different destinations.
