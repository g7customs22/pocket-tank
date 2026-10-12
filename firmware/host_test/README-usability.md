# Landscape usability review

Isolated production renderer/compositor at 640x172, with synthetic tanks. No device
state was changed. `usability_host.c` generates PPM screenshots in outputs/ui-review;
convert with ImageMagick. This is a deterministic input/render harness, not an
interactive hardware session or an assessment of real capacitive-touch sensitivity.

## Findings and review status

1. **Show care as minutes:seconds.** Confirmed renderer floors to whole minutes.
   Healthy, paused and decay screens are legible and name the blocked fish, but
   59 seconds of progress still displays 0 / 15 MIN. Smallest change: show 0:59 /
   15:00, retaining PAUSED/DECAY. This directly addresses apparent inactivity.
2. **Keep an unaffordable item open and explain the shortfall.** A CASTLE costs
   150; with balance 4 the amount and YOU HAVE 4 are very dim. Input test confirms
   touching disabled UNLOCK dismisses the modal. Smallest change: keep it open,
   show NEED 146 MORE in readable ink. Buying at exactly 150 is visibly enabled.
3. **Two-line scrolling: subsequently approved and implemented.** UP/DOWN now
   move 40 pixels per tap; all 196 pixels require five taps (previously ten). The help page and earnings list span multiple
   views, and fish-card details continue below the initial view. The last step clamps to the bottom, retaining odd final lines. Selection
   arrows are unchanged; only viewport scroll uses the larger step. The rails
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
A short release now hides/restores controls and page/card including modal and
scroll after a 350ms double-press window. Double press switches White/UV style.
Release debounce is 30ms and minimum press is 50ms; style can change while off.
Long at 700ms toggles light once. Clean view starts unobstructed and maps every
pixel of the full 640x172 panel into the aquarium, without hidden control hitboxes.
Fish taps and decoration holds stay in the aquarium while clean view is active. Chord/reset and
protected pages cancel singles; boot-held keys and bounce do not act. There is no
hold-progress visual indicator yet; adding one is an optional recommendation.
Only the subsequently approved two-line scrolling recommendation was applied;
other review recommendations remain for review.
