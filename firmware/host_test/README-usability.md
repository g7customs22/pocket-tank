# Landscape usability review

Isolated production renderer/compositor at 640x172, with synthetic tanks. No device
state was changed. `usability_host.c` generates PPM screenshots in outputs/ui-review;
convert with ImageMagick. This is a deterministic input/render harness, not an
interactive hardware session or an assessment of real capacitive-touch sensitivity.

## Recommendations for review (not implemented)

1. **Show care as minutes:seconds.** Confirmed renderer floors to whole minutes.
   Healthy, paused and decay screens are legible and name the blocked fish, but
   59 seconds of progress still displays 0 / 15 MIN. Smallest change: show 0:59 /
   15:00, retaining PAUSED/DECAY. This directly addresses apparent inactivity.
2. **Keep an unaffordable item open and explain the shortfall.** A CASTLE costs
   150; with balance 4 the amount and YOU HAVE 4 are very dim. Input test confirms
   touching disabled UNLOCK dismisses the modal. Smallest change: keep it open,
   show NEED 146 MORE in readable ink. Buying at exactly 150 is visibly enabled.
3. **Reduce scrolling effort.** Production UP/DOWN move 20 pixels per tap; all
   196 pixels require ten taps. The help page and earnings list span multiple
   views, and fish-card details continue below the initial view. Smallest change:
   larger steps or held-button repeat, tested to leave overlap/context. The rails
   are generous 76x48 / 80x44 targets. Compact content buttons are generally 32px
   high with hit slop; actual finger comfort still requires device evaluation.
4. **Label fish care meters numerically.** Fish detail uses icons and bars while
   breeding uses exact 4/4/3 thresholds. Suggest numeric hunger/energy/stress only
   on the detail card. This is inferred benefit, not a demonstrated input failure.

## Screens and verified behavior

- menu-top/bottom: correct tiger-barb portraits, dark unearned badges; menu-bottom
  repeats Settings/Upgrades/Close while rails already offer Settings/Shop/Tank.
  This duplicated vocabulary is visible; standardizing it is a secondary suggestion.
- care-healthy/paused/decay/ready: total retained, named blockers, 15-minute cap.
- care-how-top/bottom: full explanatory text reachable by scrolling.
- shop-poor/unaffordable/affordable/rewards: actual doubled amounts 4, 10/20/50,
  40, 30, 50; no price change. Last rewards require scrolling.
- fish-detail: native camera and stats composited from clean aquarium pixels.
- milestone-detail: locked FIRST MEAL FROM YOU shows NOT YET.
- species: 212x44 choice targets, current tiger barb outlined; filtered to habitat.
- tank-switcher: each tank explicitly marked ACTIVE or PAUSED; 432x44 row targets.
  Tests assert the species choice and active tank row map to their intended actions.

## Approved button change

Physical BOOT/GPIO0 was the existing light key. PWR remains sleep/power.
A single press now hides/restores page/card including modal and scroll. Long at
700ms toggles light once. Single feedback waits 350ms after release to arbitrate
possible doubles. Double is reserved (no action pending choice). Chord/reset and
protected pages cancel singles; boot-held keys and bounce do not act. There is no
hold-progress visual indicator yet; adding one is an optional recommendation.
No new review recommendations above were applied to production UI.
