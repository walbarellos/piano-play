# UI — Wireframes

## Menu Principal

```
┌──────────────────────────────────┐
│            ABNT PIANO            │
│                                  │
│         > Free Play              │
│           Song Select            │
│           Configurações          │
│                                  │
│  Perfil: Ohr   Peças: 3/12      │
└──────────────────────────────────┘
```

## Song Select

```
┌──────────────────────────────────────────┐
│  SONG SELECT                              │
│  ┌────────────────────────────────────┐  │
│  │ Prelude Op.28 No.4 — Chopin         │  │
│  │ Easy ✓  Normal ✓  Hard 🔒  Expert 🔒 │  │
│  │ ▂▃▅▇ densidade (Hard)                │  │
│  └────────────────────────────────────┘  │
│  ┌────────────────────────────────────┐  │
│  │ Für Elise — Beethoven                │  │
│  │ Easy ✓  Normal 🔒 Hard 🔒 Expert 🔒   │  │
│  └────────────────────────────────────┘  │
│                                            │
│  [ENTER] jogar   [←→] dificuldade         │
└──────────────────────────────────────────┘
```

## Song Mode (gameplay)

```
┌──────────────────────────────────────────┐
│ Score: 12,340   Combo: 24   Acc: 96.2%   │  ← Zona HUD
│                                            │
│        [D]                                │
│              [F][G][J] (acorde, conectado) │  ← Zona de Leitura
│  [S]                                      │     (somente leitura,
│                                            │      ADR-10 / RF26)
│──────────────────────────────────  ← hit line
│  Q  W  E  R  T  Y  U  I  O  P             │
│   A [S][D][F] G [J] K  L                  │  ← Zona de Resposta:
│    Z  X  C  V  B  N  M                    │    teclado visual (RF28),
│                                            │    glow no acerto (RF26)
└──────────────────────────────────────────┘
```

- `[X]` = bloco de nota descendo com a letra impressa (RF29); largura
  = duração; posição X alinhada à tecla correspondente no teclado
  visual embaixo.
- Barra conectando blocos = `ChordGroup` (acorde, RF18) — todas devem
  ser pressionadas juntas.
- Linha `──` = hit line, fronteira entre Zona de Leitura e Zona de
  Resposta.
- Teclado visual embaixo (RF28) = as 3 fileiras reais do ABNT
  (`mapeamento-teclado.md`); teclas em uso no `Chart` ficam destacadas,
  glow aparece nelas no momento do acerto (aqui indicado por `S̈ D̈ F̈ G Ḧ J̈`
  — apenas ilustrativo do destaque, não um caractere real da UI).

## Resultado

```
┌──────────────────────────────────┐
│              GRADE: A             │
│                                   │
│  Perfect: 142   Great: 30         │
│  Good: 8        Miss: 4           │
│  Max Combo: 88   Acc: 94.1%       │
│                                   │
│  [R] repetir  [ESC] voltar        │
└──────────────────────────────────┘
```
