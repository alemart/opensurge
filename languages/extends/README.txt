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
// The key benefit of using language extensions reside in the fact that your
// modifications are kept separated from the files that are shipped with the
// engine. When upgrading the engine, you will typically overwrite the default
// .lng files. However, if you use language extensions, you will be able to
// keep your modifications!
//
//
// HOW TO USE
//
// Suppose that you want to extend english.lng (the default language file).
//
// 1. Copy this README to languages/extends/english.lng
// 2. Add and/or modify the entries you wish as in the examples below
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
