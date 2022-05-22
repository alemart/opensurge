//
//
// LANGUAGE EXTENSIONS
//
//
// Language extensions are used to add and modify entries of language files
// without altering those files directly.
//
// Langugage extensions are .lng files located in languages/extends/ (this
// is a special folder). Such .lng files must have the same name as the files
// you intend to extend. Example: if you want to extend languages/english.lng,
// then you need to write your entries to langugages/extends/english.lng.
//
// The key benefit of using language extensions is the fact that your
// modifications are kept separated from the files that are shipped with the
// engine. When upgrading the engine, you will typically overwrite the default
// .lng files. If you use language extensions however, you will be able to keep
// your modifications!
//
//
// HOW TO USE
//
// Suppose you want to extend the english.lng file.
//
// 1. Copy this README to languages/extends/english.lng
// 2. Add or modify entries as in the examples below
// 3. Launch the engine and select the English language
//
//


//
// EXAMPLE 1
// Replace "Power" by "Coins" in the Heads-Up Display
//
HUD_POWER                   "Coins"


//
// EXAMPLE 2
// Add a new entry to the language file
//
MY_NEW_ENTRY                "My new entry works!!!"
