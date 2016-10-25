[![Appveyor Build status](https://ci.appveyor.com/api/projects/status/dkl49xt1j7y78km4/branch/master?svg=true)](https://ci.appveyor.com/project/hymerman/foo-bestversion/branch/master)

foo_bestversion
===============

A simple component that can pick the best version of a track from your library.

"Best" is a fuzzy notion that takes into account a number of things, such as bitrate, title match, musicbrainz release type, and playcount. See BestVersion.cpp for details.

This is useful when you have a playlist containing some tracks, and there are better alternatives to some of those tracks in your library. For example, you could have an old playlist pointing to low bitrate versions of some tracks, which you have recently added higher bitrate versions of.

Really though, the behaviour is most useful in conjunction with some other features:

Also provided is a command to grab the most popular tracks for a selected artist from last.fm, and generate a playlist using tracks from your library, using the same rules the best version calculation does. This avoids the main problem with foo_softplaylists, which is that it picks 'live' versions of tracks, or tracks from VA compilation albums, purely because they have a slightly better bitrate than the track on the original album, for example.

There's also a command for Last.fm 'similar tracks'.

In future, I would also like to support XSPF playlists (like foo_softplaylists), dead item fixers (like foo_playlist_revive), and text parsers (to e.g. allow you to paste in a bunch of track names and have a playlist generated from them).

Usage
=====

Fairly straightforward!

Right click -> Best Version -> Replace with best version of track

Right click -> Last.fm -> XXX's top tracks
Right click -> Last.fm -> Get tracks similar to XXX

Download
========

Built versions will be available here:

https://github.com/hymerman/foo_bestversion/releases

Pdb files are also provided in case you fancy debugging releases yourself.

Other Useful Links
==================

Discussion thread:

http://www.hydrogenaudio.org/forums/index.php?showtopic=100019

Github page:

https://github.com/hymerman/foo_bestversion
