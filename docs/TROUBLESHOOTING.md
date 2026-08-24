# HolyScreen troubleshooting

[Português do Brasil](TROUBLESHOOTING.pt-BR.md) · [Operator manual](OPERATOR_MANUAL.md)

Export sanitized diagnostics from Maintenance before changing a failing
configuration. Never publish a database, password, token or protected media in
an issue.

## External display is missing or on the wrong monitor

1. Confirm the operating system detects the display in extended mode.
2. Open **Settings > Screens > Identify** and compare the physical labels.
3. Re-enable the output and assign its role again after a GPU, dock or cable
   change.
4. Close and reopen HolyScreen only after the OS display layout is stable.

For mixed DPI, keep each display at a supported system scale and restore the
operator layout if controls become inaccessible.

## Video is black, corrupt or has no audio

- Play another known-good H.264/AAC MP4 to distinguish a routing problem from
  a codec/container problem.
- Check that media is enabled for the intended output and that Blackout is off.
- Rescan the library and verify the source file was not moved or deleted.
- Check the selected audio device, mute button and system mixer.
- Transcode unsupported media outside HolyScreen; the packaged Qt multimedia
  backend determines available codecs.

Do not repeatedly start a corrupt file during a live service. Remove it from
the active playlist and investigate afterward.

## Audio uses the wrong device or drops out

Select the intended output again after connecting USB/HDMI audio. Avoid
changing the operating-system default device during playback. Stop the item,
select the stable device and restart it. Check CPU/disk pressure if a known-good
local WAV or MP3 also drops out.

## Remote page does not connect

1. Confirm the server is enabled and the phone is on the same trusted network.
2. Use the current QR/URL; DHCP can change the computer address.
3. Check the local firewall and selected IPv4 interface.
4. Re-enter the password after a session revocation or eight-hour expiry.
5. Reload the PWA after Wi-Fi returns; it obtains a fresh state snapshot.

Do not solve this by forwarding the port to the public internet. See
[Remote API security](REMOTE_API.md).

## OBS does not see Broadcast

Confirm an output is assigned to Broadcast and enabled. Re-select the correct
window/screen source in OBS after changing monitor topology. Match the
Broadcast profile resolution and safe area, then test motion and audio before
the service. HTTP/OBS integration status is available in Integrations.

## Bible reference is not found

Confirm a translation is imported and enabled. Try selecting book/chapter in
the Bible area to separate parser input from missing content. Reimport only
from a trusted, licensed source; interrupted imports are staged and should not
replace a valid translation. See [Bible import](BIBLE_IMPORT.md).

## Database cannot open, save or restore

- Stop presentation activity and create a copy of the application-data folder.
- Check free disk space and filesystem permissions.
- Use the newest known-good backup; never overwrite the only backup.
- Keep the original database when reporting a migration failure.
- After forced termination, let HolyScreen recovery checks finish before
  restoring manually.

## Update check fails

The check needs HTTPS access to `api.github.com` and never installs anything.
Retry after restoring internet access or open the official Releases page
manually. Do not replace the endpoint or accept packages from another host.

## HolyScreen appears to remain running after close

Wait briefly for media, remote and integration shutdown. If the process remains
after the operator window closes, capture diagnostics and the process state,
then terminate it with the operating system. On the next launch, avoid starting
live outputs until database recovery has completed.
