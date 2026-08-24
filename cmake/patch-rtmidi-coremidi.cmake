# Corrige um defeito do RtMidi 6.0.0 no backend CoreMIDI.
#
# `MidiInCore::getCoreMidiClientSingleton` e a variante de saída são declaradas
# `throw()` — isto é, noexcept — mas chamam `error()`, que lança `RtMidiError`
# quando o tipo não é aviso. Lançar de dentro de uma função noexcept chama
# `std::terminate`, então qualquer falha de `MIDIClientCreate` encerra o
# processo com SIGABRT antes que o chamador possa tratar o erro.
#
# Isso acontece em máquinas sem servidor MIDI disponível, em sessões
# restritas e nos runners de CI do macOS, e derrubava o HolyScreen na
# inicialização. Remover a especificação permite que a exceção chegue ao
# `try`/`catch` que o RtMidiTransport já mantém, e o aplicativo apenas informa
# que não há backend MIDI.
#
# Aplicado sobre a árvore extraída da versão fixada por checksum. É idempotente:
# rodar de novo não encontra nada a substituir.

set(rtmidi_source "RtMidi.cpp")
if(NOT EXISTS "${rtmidi_source}")
    message(FATAL_ERROR "Não encontrei ${rtmidi_source} para corrigir.")
endif()

file(READ "${rtmidi_source}" contents)
set(needle "getCoreMidiClientSingleton(const std::string& clientName) throw()")
set(replacement "getCoreMidiClientSingleton(const std::string& clientName)")
string(REPLACE "${needle}" "${replacement}" patched "${contents}")

if(patched STREQUAL contents)
    message(STATUS "RtMidi: correção do CoreMIDI já aplicada ou desnecessária.")
else()
    file(WRITE "${rtmidi_source}" "${patched}")
    message(STATUS "RtMidi: removida a especificação noexcept dos getters do cliente CoreMIDI.")
endif()
