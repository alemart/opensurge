// -----------------------------------------------------------------------------
// File: launch_url.ss
// Description: a function object that launches a URL
// Author: Alexandre Martins <http://opensurge2d.org>
// License: MIT
// -----------------------------------------------------------------------------
using SurgeEngine.Web;

//
// Launch URL is a function object that launches a URL
//
// Arguments:
// - url: string. The URL to be launched. Make sure to include
//                the protocol (http://, https://, mailto:)
//                e.g., "http://opensurge2d.org"
//
object "Launch URL"
{
    fun call(url)
    {
        Web.launchURL(url);
    }
}