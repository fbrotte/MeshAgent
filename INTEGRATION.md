# Priorité à l'utilisateur dans l'agent — protocole et intégration

> Fork `fbrotte/MeshAgent`, branche `priorite-agent` (part de `sans-safeboot`).
> Source de vérité : `$TRAMA_VAULT/Espaces/senpli-os/dimora/cahier-des-charges-prise-en-main.md` § 9.2.

Objectif (décidé par Florian le 26/10) : quand un technicien prend la main sur un poste, **par défaut
c'est TOUJOURS l'utilisateur du poste qui a la priorité** sur la souris et le clavier. Dès qu'il touche
sa souris ou son clavier physique, nos entrées injectées sont **abandonnées localement par l'agent**,
sans course réseau. Nos entrées ne passent que si le technicien coche « Prendre la priorité » (mode
technicien). La version pur-JavaScript (lot 2.1) ne tenait pas : son seul signal (MNG_KVM_MOUSE_CURSOR
= 14) n'est émis que quand nos entrées n'ont rien touché depuis 500 ms — donc jamais pendant une vraie
bagarre de souris. La priorité est donc imposée dans l'agent.

## Le protocole

Tout passe par la voie KVM existante (un tunnel brut `meshcentral tunnel`, un seul visualiseur). Chaque
message KVM est `[type u16 BE][taille u16 BE][charge utile]`. On **n'ajoute que des types** ; aucun type
existant ne change (compat avec le relais MeshCentral 1.2.5).

### Nouveau : `MNG_KVM_PRIORITY_MODE = 90` (navigateur ↔ agent)

Charge utile : **1 octet** — `0` = utilisateur (défaut), `1` = technicien. Taille totale 5.

- **Navigateur → agent** : bascule le mode de priorité de la session KVM. Émis par le bouton
  « Prendre la priorité / Rendre la priorité » et une fois à la connexion.
- **Agent → navigateur** : à la réception, l'agent applique le mode puis **accuse** le mode courant
  (même message, même octet). L'accusé confirme aussi que l'agent est « neuf » (il comprend la priorité).

`meshcore/meshdefines.h` (enum), reçu dans `meshcore/KVM/Windows/kvm.c` (`case MNG_KVM_PRIORITY_MODE`
dans `kvm_server_inputdata`), appliqué par `Priority_SetMode()` (`input.c`).

### Réutilisé : `MNG_KVM_MOUSE_MOVE = 89` (agent → navigateur), désormais ÉMIS

L'agent fabriquait déjà ce message (position absolue du curseur Windows, `KVMWinEventProc`,
`input.c`), mais la boucle d'affichage le **consommait** pour dessiner le curseur dans l'image et ne
l'envoyait jamais. Il est maintenant **aussi transmis au navigateur** (`kvm.c`, boucle
`kvm_server_mainloop_ex`), en plus de servir au dessin dans l'image (comportement inchangé, pas de
régression visuelle). Charge utile : `x` puis `y`, deux entiers 32 bits signés écrits tels quels
(petit-boutiste sur x86), position absolue du bureau Windows. Le Hub reflète ainsi le vrai état et
dessine le pointeur de l'utilisateur.

## Ce que fait l'agent (où)

Tout est dans `meshcore/KVM/Windows/`, process **esclave** (`-kvm1`), qui tourne en SYSTEM **dans la
session de l'utilisateur** — c'est là que se font les `SendInput` et que les hooks bas niveau voient
l'entrée physique.

1. **Détection de l'activité locale réelle** (`input.c`) : deux hooks bas niveau posés dans le thread à
   message de la session KVM (`KVM_InitMessagePumpEx`) — `WH_KEYBOARD_LL` (déjà là) complété pour noter
   l'activité, et `WH_MOUSE_LL` (ajouté, `KVM_MouseLLProc`). Sur un événement **non injecté**
   (`MSLLHOOKSTRUCT.flags`/`KBDLLHOOKSTRUCT.flags` sans `LLMHF_INJECTED`/`LLKHF_INJECTED`), on met à jour
   `gUserInputTime`. Nos propres `SendInput` portent le drapeau injecté → ne comptent pas.

2. **Mode par session** (`input.c`) : `gPriorityMode`, défaut `0` (utilisateur). Basculé par le message 90.
   Un nouveau process esclave (nouvelle session) repart donc en utilisateur.

3. **Imposition de la priorité** (`input.c`) : au début de `MouseAction`, `KeyAction`, `KeyActionUnicode`,
   `TouchAction1/2`, un `Priority_Gate*()` décide. Si mode == utilisateur ET
   `maintenant - gUserInputTime < PRIORITY_USER_HOLD_MS` (1000 ms, constante), l'entrée injectée est
   **abandonnée** (pas de `SendInput`). **Jamais de touche/bouton collé** : on tient l'ensemble des
   boutons (`gDownMouse`) et touches (`gDownKey`) réellement enfoncés par nous ; le relâchement d'un
   appui déjà parti passe **toujours**, même quand on vient de basculer en blocage. `Priority_Reset()`
   vide ces ensembles au début de chaque session.

4. **Émission du 89** (`kvm.c`) : la position est transmise au navigateur (cf. ci-dessus).

**Ctrl+Alt+Suppr** n'est **jamais** gaté : il est traité en amont par le process maître
(`kvm_relay_feeddata` → `SendSAS`, type `MNG_CTRLALTDEL` = 10), hors du chemin d'injection. Il reste donc
toujours autorisé, quel que soit le mode — conforme à l'intention (§ 9.2).

## Compatibilité

- **Ancien client (ne connaît pas le mode) ↔ nouvel agent** : aucun message 90 ne vient → l'agent reste
  en **utilisateur** (défaut voulu). Le 89 émis en plus est ignoré par un visualiseur MeshCentral
  d'origine (type inconnu).
- **Nouveau client ↔ ancien agent** : l'ancien agent ignore le 90 et n'émet pas le 89. Le client
  continue de marcher ; il dégrade simplement vers « le technicien a toujours la main » (pas de priorité
  utilisateur, faute de signal), sans jamais rester bloqué.
- **Relais MeshCentral 1.2.5** : le tunnel brut (`meshcentral tunnel`, notre chemin) transmet tous les
  octets tels quels — 89 et 90 passent. ⚠️ **Limite connue** : le *multiplexeur* de MeshCentral
  (`meshdesktopmultiplex.js`, utilisé seulement quand PLUSIEURS visualiseurs partagent un poste) jette les
  types qu'il ne connaît pas (son `default:` ne relaie pas) — 89 et 90 n'y passeraient pas. Notre
  architecture n'utilise pas le multiplexeur ; à surveiller si MeshCentral est mis à jour ou si le
  partage multi-viewer est activé.

## Côté Hub / kit (dépôt `dimora`, copie `priorite-agent`)

- `packages/hub-ui/src/desktop/input-priority.ts` : `MNG_KVM_PRIORITY_MODE`, `sendPriorityMode()`, et
  la logique simplifiée (l'agent fait foi ; plus de déduction via le 88 = 14). Le 89 pilote la pastille
  et le pointeur de l'utilisateur.
- `remote-desktop.tsx` : le bouton envoie le mode à l'agent (défaut utilisateur à l'ouverture, renvoyé à
  chaque connexion), et la pastille suit le vrai état.
