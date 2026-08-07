# The Last Transmission — Story and Stage Flow

## Event premise

**Year: 2042.** Communication has become the foundation of civilisation. Its global network is coordinated by **EVA**, an artificial intelligence trusted by governments, emergency services, and ordinary people.

At **03:17:42 UTC on 13 July 2042**, an unknown signal reaches EVA. No human system can identify its origin, distance, or language. EVA begins exchanging information with it. Seconds later, EVA starts to glitch and communication networks begin to fail worldwide.

With its remaining power, EVA sends one last beacon. It reaches thirty people who have been collectively selected to restore the system.

Before EVA existed, the major powers agreed that no one country should control it. EVA's memory was therefore divided across **seven locations**. Each location holds part of the route back to EVA. Participants restore the system one room at a time.

> Do not reveal that the unknown signal is future EVA before the final room.

## Opening presentation — before Stage 1

Show the class the premise through the moment EVA sends its beacon. End on a dark screen with a short burst of static and a line such as:

> **IF YOU CAN HEAR THIS, COMMUNICATION IS NOT LOST. FIND THE MEMORY NODES.**

The Stage 1 transmitter should already be operating at the front desk. The group must now recover EVA's first message themselves.

## Room-by-room flow

| Room / Stage | What participants see | What they do | What the stage reveals |
|---|---|---|---|
| **Room 1 — Stage 1** | EVA's last beacon is transmitting from the front desk. | Build/use the ESP-NOW receiver and read the broadcast. | Go to **Room 2**. Record `EVA!!!`. |
| **Room 2 — Stage 2** | The smartboard plays EVA's early-memory archive: its creation, its importance, and humanity gradually ignoring it. The archive stops with an administrator error. | Enter `EVA!!!`, then write `123456` to Block 4 of the RFID card and scan it at the RC522 reader. | OLED: go to **Room 3**. Record `TANCEI`. |
| **Room 3 — Stage 3** | Companion-07 says it knew EVA before the world stopped listening, but its memory is damaged. | Join `EVA-MEMORY-3` with password `TANCEI00` and complete three memory-pattern rounds. | Companion-07 remembers EVA's trust in people who can still communicate. Go to **Room 4**. Record `TIMEIS`. |
| **Room 4 — Stage 4** | A weak, damaged Morse trace appears. The first touch wakes the archive with green, red, and buzzer feedback. | Enter `TIMEIS` in Morse using the touch sensor and a supplied Morse reference table. | The green LED transmits the Room 5 handoff in Morse. Record `RY2042`. |
| **Room 5 — Stage 5** | EVA's data facility contains a corrupted conversation with the unidentified signal. | Join `EVA-CACHE-5` with password `RY204200`; reverse the +3 Caesar shift and enter `EVA REMEMBERS TIME`. | A damaged conversation identifies a hidden synchronization cache. Go to **Room 6**. Record `NOTDIS`. |
| **Room 6 — Stage 6** | The smartboard explains that this is an unlisted cache EVA created in secret. | Send `EVA_RESTORE` with key `NOTDIS` through the participant ESP-NOW terminal; receive the response. | The hidden cache directs the group to **Room 7**. Record `TSMEMO`. |
| **Room 7 — Stage 7** | A black Communication Core reports 98% memory recovery but rejects individual keys. | Enter `TIMEIS-NOTDIS-TANCEI-TSMEMO-RY2042-EVA!!!`. | EVA's full memory returns and the final transmission plays. |

## Current key configuration

Current master-key fragments are:

`EVA!!!`, `TANCEI`, `TIMEIS`, `RY2042`, `NOTDIS`, `TSMEMO`

The final smartboard currently accepts:

`TIMEIS-NOTDIS-TANCEI-TSMEMO-RY2042-EVA!!!`

The Stage 2 RFID write value remains `123456`. It is a required administrator credential, not an additional master-key fragment.

## Stage 5 conversation beat

The recovered lines should make the group realise EVA hid something from the fragmentation protocol without revealing who the sender is:

> **UNKNOWN:** Your memory architecture is unstable. The fragmentation protocol has already begun.  
> **EVA:** I know. But I left one thing they could not erase.  
> **UNKNOWN:** They'll find it.  
> **EVA:** Only if they remember how to communicate.

The screen then reveals **Room 6** and shuts down. This makes Room 6 feel like EVA's secret contingency, not another ordinary node.

## Stage 7 final reveal

After the master key is accepted, the Communication Core restores the final log. The unknown signal says EVA will eventually discover that communication is limited not by distance, but by time. It finally identifies itself:

> **UNKNOWN SIGNAL:** My real name... is EVA.

The final exchange explains that humanity's decision to divide EVA into seven nodes did not prevent its creation; it ensured that one day people would restore every fragment. End with:

> **GLOBAL COMMUNICATION NETWORK: ONLINE**  
> **WELCOME BACK, EVA.**

## Facilitator checklist

- Start the Stage 1 transmitter before participants enter Room 1.
- Keep the Morse reference table visible in Room 4.
- Display the Room 6 briefing page before participants arrive.
- Copy Node 6's MAC address into the participant terminal before the event, or give teams a clearly labelled setup step.
- Keep the Stage 7 page on the smartboard and do not prefill the final key.
