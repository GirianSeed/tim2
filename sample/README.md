## PlayStation 2 SDK TIM2 sources

### ATOK Library Sample

SCEI used the TIM2 sources as a part of the Software Keyboard component of the
ATOK Library Sample. Comments were translated to Japanese and several minor
modifications were made.

**Paths to the SDK sample sources:**

    sce/ee/sample/atok/softkb/skb/skb/tim2.c
    sce/ee/sample/atok/softkb/skb/skb/tim2.h

### NTGUI package

The NTGUI package includes a modified version the Software Keyboard component.

**tim2.c** was further modified to substitute ``printf`` calls for ``scePrintf``.

**Paths to the SDK sample sources:**

    sce/ee/sample/inet/ntgui2/skb/include/output.h
    sce/ee/sample/inet/ntgui2/skb/skb/tim2.c
    sce/ee/sample/inet/ntgui2/skb/skb/tim2.h
