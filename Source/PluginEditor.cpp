#include "PluginEditor.h"

#if JucePlugin_Build_Standalone
 #include <juce_audio_plugin_client/Standalone/juce_StandaloneFilterWindow.h>
#endif

namespace ui
{
namespace
{
juce::Font font (float size, bool bold = true, float kerning = 0.0f)
{
    return juce::Font (juce::FontOptions (size, bold ? juce::Font::bold : juce::Font::plain))
        .withExtraKerningFactor (kerning);
}

void drawScrew (juce::Graphics& g, juce::Point<float> c, float r)
{
    g.setGradientFill (juce::ColourGradient (juce::Colour (0xffd9d9d9), c.x - r, c.y - r,
                                             juce::Colour (0xff6b6b6b), c.x + r, c.y + r, false));
    g.fillEllipse (c.x - r, c.y - r, r * 2, r * 2);
    g.setColour (juce::Colour (0xff3a3a3a));
    g.drawLine (c.x - r * 0.7f, c.y, c.x + r * 0.7f, c.y, 1.2f);
}

void drawLed (juce::Graphics& g, juce::Point<float> c, float r, bool on, juce::Colour colour = col::ledRed)
{
    if (on)
    {
        g.setGradientFill (juce::ColourGradient (colour.withAlpha (0.45f), c.x, c.y,
                                                 colour.withAlpha (0.0f), c.x + r * 4, c.y, true));
        g.fillEllipse (c.x - r * 4, c.y - r * 4, r * 8, r * 8);
    }
    g.setColour (juce::Colours::black);
    g.fillEllipse (c.x - r - 1.5f, c.y - r - 1.5f, (r + 1.5f) * 2, (r + 1.5f) * 2);
    g.setGradientFill (juce::ColourGradient (on ? colour.brighter (0.6f) : colour.darker (2.2f), c.x - r * 0.4f, c.y - r * 0.4f,
                                             on ? colour : colour.darker (3.0f), c.x + r, c.y + r, true));
    g.fillEllipse (c.x - r, c.y - r, r * 2, r * 2);
}

void drawJack (juce::Graphics& g, juce::Point<float> c)
{
    g.setColour (juce::Colour (0xff101010));
    g.fillEllipse (c.x - 9, c.y - 9, 18, 18);
    g.setGradientFill (juce::ColourGradient (juce::Colour (0xffcfcfcf), c.x - 6, c.y - 6,
                                             juce::Colour (0xff5a5a5a), c.x + 6, c.y + 6, false));
    g.fillEllipse (c.x - 6.5f, c.y - 6.5f, 13, 13);
    g.setColour (juce::Colour (0xff050505));
    g.fillEllipse (c.x - 3, c.y - 3, 6, 6);
}
} // namespace

// ---------------------------------------------------------------------------
BoardLookAndFeel::BoardLookAndFeel()
{
    setColour (juce::BubbleComponent::backgroundColourId, juce::Colour (0xee111111));
    setColour (juce::BubbleComponent::outlineColourId, col::gold.withAlpha (0.6f));
    setColour (juce::TooltipWindow::textColourId, col::cream);
}

void BoardLookAndFeel::drawRotarySlider (juce::Graphics& g, int x, int y, int w, int h, float pos, float start,
                                         float end, juce::Slider& s)
{
    const auto skin = s.getProperties()["skin"].toString();
    const auto area = juce::Rectangle<float> ((float) x, (float) y, (float) w, (float) h).reduced (3.0f);
    const float d = juce::jmin (area.getWidth(), area.getHeight());
    const auto c = area.getCentre();
    const float r = d * 0.5f;
    const float angle = start + pos * (end - start);

    auto pointer = [&] (float inner, float outer, float thickness, juce::Colour colour)
    {
        juce::Path p;
        p.addRoundedRectangle (-thickness * 0.5f, -outer, thickness, outer - inner, thickness * 0.5f);
        g.setColour (colour);
        g.fillPath (p, juce::AffineTransform::rotation (angle).translated (c));
    };

    // drop shadow
    g.setColour (juce::Colours::black.withAlpha (0.45f));
    g.fillEllipse (c.x - r + 2, c.y - r + 4, d, d);

    if (skin == "amp")
    {
        // tick ring
        g.setColour (juce::Colour (0xff2a2014));
        for (int i = 0; i <= 10; ++i)
        {
            const float a = start + (end - start) * (float) i / 10.0f;
            const juce::Point<float> p1 = c.getPointOnCircumference (r + 1.0f, a), p2 = c.getPointOnCircumference (r - 3.0f, a);
            g.drawLine ({ p1, p2 }, i % 5 == 0 ? 1.8f : 1.0f);
        }
        const float kr = r - 6.0f;
        g.setGradientFill (juce::ColourGradient (juce::Colour (0xff3a3a3a), c.x, c.y - kr, juce::Colour (0xff0b0b0b), c.x, c.y + kr, false));
        g.fillEllipse (c.x - kr, c.y - kr, kr * 2, kr * 2);
        const float cr = kr * 0.62f;
        g.setGradientFill (juce::ColourGradient (juce::Colour (0xfff3dca0), c.x - cr, c.y - cr, juce::Colour (0xff9a7a38), c.x + cr, c.y + cr, false));
        g.fillEllipse (c.x - cr, c.y - cr, cr * 2, cr * 2);
        pointer (cr * 0.15f, kr - 1.0f, 2.6f, juce::Colour (0xff111111));
        return;
    }

    if (skin == "pedalLight")
    {
        g.setGradientFill (juce::ColourGradient (juce::Colour (0xff5d6268), c.x, c.y - r, juce::Colour (0xff23262a), c.x, c.y + r, false));
        g.fillEllipse (c.x - r, c.y - r, d, d);
        g.setColour (juce::Colour (0xff15171a));
        g.drawEllipse (c.x - r, c.y - r, d, d, 1.2f);
        const float tr = r * 0.72f;
        g.setGradientFill (juce::ColourGradient (juce::Colour (0xff474b50), c.x - tr, c.y - tr, juce::Colour (0xff2b2e32), c.x + tr, c.y + tr, false));
        g.fillEllipse (c.x - tr, c.y - tr, tr * 2, tr * 2);
        pointer (r * 0.2f, r - 2.0f, 3.0f, juce::Colours::white);
        return;
    }

    if (skin == "small")
    {
        g.setGradientFill (juce::ColourGradient (juce::Colour (0xff4a4a4a), c.x, c.y - r, juce::Colour (0xff151515), c.x, c.y + r, false));
        g.fillEllipse (c.x - r, c.y - r, d, d);
        g.setColour (juce::Colour (0xff080808));
        g.drawEllipse (c.x - r, c.y - r, d, d, 1.0f);
        pointer (r * 0.25f, r - 2.0f, 2.4f, col::gold);
        return;
    }

    // "pedal": black knurled knob with white indicator
    g.setColour (juce::Colour (0xff050505));
    g.fillEllipse (c.x - r, c.y - r, d, d);
    g.setColour (juce::Colour (0xff2c2c2c));
    for (int i = 0; i < 36; ++i)
    {
        const float a = angle + juce::MathConstants<float>::twoPi * (float) i / 36.0f;
        g.drawLine ({ c.getPointOnCircumference (r - 0.5f, a), c.getPointOnCircumference (r - 4.0f, a) }, 1.0f);
    }
    const float tr = r * 0.74f;
    g.setGradientFill (juce::ColourGradient (juce::Colour (0xff3b3b3b), c.x - tr, c.y - tr, juce::Colour (0xff0e0e0e), c.x + tr, c.y + tr, false));
    g.fillEllipse (c.x - tr, c.y - tr, tr * 2, tr * 2);
    pointer (r * 0.25f, r - 3.0f, 3.0f, juce::Colours::white);
}

void BoardLookAndFeel::drawButtonBackground (juce::Graphics& g, juce::Button& b, const juce::Colour&, bool over, bool down)
{
    auto r = b.getLocalBounds().toFloat().reduced (1.0f);
    g.setGradientFill (juce::ColourGradient (juce::Colour (down ? 0xff1a1a1a : 0xff3a3a3a), r.getX(), r.getY(),
                                             juce::Colour (0xff141414), r.getX(), r.getBottom(), false));
    g.fillRoundedRectangle (r, 4.0f);
    g.setColour (over ? col::gold : col::gold.withAlpha (0.45f));
    g.drawRoundedRectangle (r, 4.0f, 1.0f);
}

void BoardLookAndFeel::drawButtonText (juce::Graphics& g, juce::TextButton& b, bool, bool)
{
    g.setColour (col::cream);
    g.setFont (font (11.0f, true, 0.08f));
    g.drawText (b.getButtonText(), b.getLocalBounds(), juce::Justification::centred);
}

// ---------------------------------------------------------------------------
Knob::Knob (juce::AudioProcessorValueTreeState& s, const juce::String& paramId, const juce::String& skin)
    : attachment (s, paramId, *this)
{
    setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
    setRotaryParameters (juce::degreesToRadians (225.0f), juce::degreesToRadians (495.0f), true);
    setPopupDisplayEnabled (true, true, nullptr, 1200);
    setMouseCursor (juce::MouseCursor::PointingHandCursor);
    setDoubleClickReturnValue (true, s.getParameter (paramId)->convertFrom0to1 (s.getParameter (paramId)->getDefaultValue()));
    getProperties().set ("skin", skin);
}

// ---------------------------------------------------------------------------
Footswitch::Footswitch (juce::AudioProcessorValueTreeState& s, const juce::String& paramId)
    : juce::Button ("footswitch"), attachment (s, paramId, *this)
{
    setClickingTogglesState (true);
    setMouseCursor (juce::MouseCursor::PointingHandCursor);
}

void Footswitch::paintButton (juce::Graphics& g, bool over, bool down)
{
    auto r = getLocalBounds().toFloat().reduced (2.0f);
    const auto c = r.getCentre();
    const float rad = juce::jmin (r.getWidth(), r.getHeight()) * 0.5f;

    g.setColour (juce::Colours::black.withAlpha (0.5f));
    g.fillEllipse (c.x - rad + 1, c.y - rad + 4, rad * 2, rad * 2);

    // hex nut
    juce::Path nut;
    nut.addPolygon (c, 6, rad, 0.0f);
    g.setGradientFill (juce::ColourGradient (juce::Colour (0xffe6e6e6), c.x - rad, c.y - rad, juce::Colour (0xff6d6d6d), c.x + rad, c.y + rad, false));
    g.fillPath (nut);

    // button cap
    const float cr = rad * (down ? 0.56f : 0.62f);
    g.setGradientFill (juce::ColourGradient (juce::Colour (over ? 0xfffafafa : 0xffeeeeee), c.x - cr, c.y - cr,
                                             juce::Colour (0xff8a8a8a), c.x + cr, c.y + cr, false));
    g.fillEllipse (c.x - cr, c.y - cr, cr * 2, cr * 2);
    g.setColour (juce::Colour (0xff4a4a4a));
    g.drawEllipse (c.x - cr, c.y - cr, cr * 2, cr * 2, 1.0f);
}

// ---------------------------------------------------------------------------
MiniSwitch::MiniSwitch (juce::AudioProcessorValueTreeState& s, const juce::String& paramId)
    : juce::Button ("switch"), attachment (s, paramId, *this)
{
    setClickingTogglesState (true);
    setMouseCursor (juce::MouseCursor::PointingHandCursor);
}

void MiniSwitch::paintButton (juce::Graphics& g, bool, bool)
{
    auto r = getLocalBounds().toFloat().reduced (2.0f);
    const bool on = getToggleState();
    g.setColour (juce::Colour (0xff0a0a0a));
    g.fillRoundedRectangle (r, 3.0f);
    auto lever = r.reduced (3.0f);
    lever = on ? lever.removeFromTop (lever.getHeight() * 0.55f) : lever.removeFromBottom (lever.getHeight() * 0.55f);
    g.setGradientFill (juce::ColourGradient (juce::Colour (0xffe0e0e0), lever.getX(), lever.getY(),
                                             juce::Colour (0xff7a7a7a), lever.getRight(), lever.getBottom(), false));
    g.fillRoundedRectangle (lever, 2.0f);
}

// ---------------------------------------------------------------------------
Selector::Selector (juce::AudioProcessorValueTreeState& s, const juce::String& paramId, juce::StringArray l,
                    juce::Colour text, bool v)
    : param (*s.getParameter (paramId)),
      attachment (param, [this] (float value) { index = juce::roundToInt (value); repaint(); }, s.undoManager),
      labels (std::move (l)), textColour (text), vertical (v)
{
    attachment.sendInitialUpdate();
    setMouseCursor (juce::MouseCursor::PointingHandCursor);
}

void Selector::paint (juce::Graphics& g)
{
    const int n = labels.size();
    auto r = getLocalBounds().toFloat();

    if (vertical)
    {
        // labels on the right, a lever slot on the left
        auto slot = r.removeFromLeft (16.0f).reduced (3.0f, 2.0f);
        g.setColour (juce::Colour (0xff050505));
        g.fillRoundedRectangle (slot, 3.0f);
        const float step = r.getHeight() / (float) n;
        const float leverY = r.getY() + step * ((float) index + 0.5f);
        g.setGradientFill (juce::ColourGradient (juce::Colour (0xffeeeeee), slot.getX(), leverY - 5,
                                                 juce::Colour (0xff808080), slot.getRight(), leverY + 5, false));
        g.fillRoundedRectangle (slot.getX() - 1.0f, leverY - 5.0f, slot.getWidth() + 2.0f, 10.0f, 3.0f);

        for (int i = 0; i < n; ++i)
        {
            g.setColour (i == index ? textColour : textColour.withAlpha (0.45f));
            g.setFont (font (10.5f, true, 0.08f));
            g.drawText (labels[i], r.withY (r.getY() + step * (float) i).withHeight (step).reduced (4.0f, 0.0f),
                        juce::Justification::centredLeft);
        }
        return;
    }

    const float w = r.getWidth() / (float) n;
    for (int i = 0; i < n; ++i)
    {
        auto cell = juce::Rectangle<float> (r.getX() + w * (float) i, r.getY(), w, r.getHeight()).reduced (3.0f);
        const bool on = i == index;
        drawLed (g, { cell.getCentreX(), cell.getY() + 6.0f }, 4.0f, on, juce::Colour (0xffffb52e));
        g.setColour (on ? textColour : textColour.withAlpha (0.5f));
        g.setFont (font (11.0f, true, 0.12f));
        g.drawText (labels[i], cell.withTrimmedTop (14.0f), juce::Justification::centred);
    }
}

void Selector::mouseDown (const juce::MouseEvent& e)
{
    const int n = labels.size();
    int newIndex;
    if (vertical)
        newIndex = (index + 1) % n;
    else
        newIndex = juce::jlimit (0, n - 1, (int) ((float) e.x / (float) getWidth() * (float) n));
    attachment.setValueAsCompleteGesture ((float) newIndex);
}

// ---------------------------------------------------------------------------
Pedal::Pedal (juce::AudioProcessorValueTreeState& s, juce::String n, juce::String sub, juce::Colour b, juce::Colour i,
              juce::String onParam, juce::StringArray knobIds, juce::StringArray knobLabels, juce::String skin)
    : state (s), name (std::move (n)), subtitle (std::move (sub)), onParamId (std::move (onParam)), body (b), ink (i),
      footswitch (s, onParamId)
{
    for (int k = 0; k < knobIds.size(); ++k)
    {
        knobs.push_back ({ std::make_unique<Knob> (s, knobIds[k], skin), knobLabels[k] });
        addAndMakeVisible (*knobs.back().knob);
    }
    footswitch.onStateChange = [this] { repaint(); };
    addAndMakeVisible (footswitch);
}

void Pedal::resized()
{
    const int n = (int) knobs.size();
    const int knobSize = 58;
    const float spacing = (float) getWidth() / (float) n;
    for (int k = 0; k < n; ++k)
        knobs[(size_t) k].knob->setBounds (juce::roundToInt (spacing * ((float) k + 0.5f)) - knobSize / 2, 34, knobSize, knobSize);

    if (selector != nullptr)
        selector->setBounds (getWidth() - 74, 118, 64, 58);

    footswitch.setBounds (getWidth() / 2 - 32, getHeight() - 92, 64, 64);
}

void Pedal::paint (juce::Graphics& g)
{
    auto r = getLocalBounds().toFloat().reduced (2.0f);
    const bool on = state.getRawParameterValue (onParamId)->load() > 0.5f;

    g.setColour (juce::Colours::black.withAlpha (0.55f));
    g.fillRoundedRectangle (r.translated (3.0f, 8.0f), 12.0f);

    g.setGradientFill (juce::ColourGradient (body.brighter (0.18f), r.getX(), r.getY(), body.darker (0.35f), r.getX(), r.getBottom(), false));
    g.fillRoundedRectangle (r, 12.0f);
    g.setColour (body.darker (0.8f));
    g.drawRoundedRectangle (r, 12.0f, 1.5f);
    g.setColour (juce::Colours::white.withAlpha (0.08f));
    g.drawRoundedRectangle (r.reduced (2.0f), 10.0f, 1.0f);

    for (auto p : { juce::Point<float> (r.getX() + 12, r.getY() + 12), { r.getRight() - 12, r.getY() + 12 },
                    { r.getX() + 12, r.getBottom() - 12 }, { r.getRight() - 12, r.getBottom() - 12 } })
        drawScrew (g, p, 4.0f);

    // knob labels
    g.setColour (ink);
    g.setFont (font (10.0f, true, 0.12f));
    const float spacing = r.getWidth() / (float) knobs.size();
    for (size_t k = 0; k < knobs.size(); ++k)
        g.drawText (knobs[k].label, juce::Rectangle<float> (r.getX() + spacing * (float) k, 18.0f, spacing, 14.0f),
                    juce::Justification::centred);

    // LED
    drawLed (g, { r.getX() + 26.0f, 132.0f }, 5.0f, on);

    // logo
    auto logo = juce::Rectangle<float> (r.getX() + 16.0f, 140.0f, r.getWidth() - (selector ? 96.0f : 32.0f), 64.0f);
    g.setColour (ink);
    g.setFont (font (40.0f, true, 0.02f));
    g.drawText (name, logo.removeFromTop (44.0f), juce::Justification::centredLeft);
    g.setColour (ink.withAlpha (0.7f));
    g.setFont (font (10.5f, true, 0.32f));
    g.drawText (subtitle, logo, juce::Justification::centredLeft);

    // side jacks
    drawJack (g, { r.getX() + 2.0f, 120.0f });
    drawJack (g, { r.getRight() - 2.0f, 120.0f });
}

// ---------------------------------------------------------------------------
AmpHead::AmpHead (juce::AudioProcessorValueTreeState& s)
    : state (s),
      channel (s, ids::ampChannel, { "CLEAN", "CRUNCH" }, juce::Colour (0xff1a140a), false),
      power (s, ids::ampOn)
{
    const char* idsList[] = { ids::ampGain, ids::ampBass, ids::ampMid, ids::ampTreble, ids::ampPresence, ids::ampMaster };
    const char* labels[] = { "GAIN", "BASS", "MIDDLE", "TREBLE", "PRESENCE", "MASTER" };
    for (int i = 0; i < 6; ++i)
    {
        knobs.push_back ({ std::make_unique<Knob> (s, idsList[i], "amp"), labels[i] });
        addAndMakeVisible (*knobs.back().knob);
    }
    addAndMakeVisible (channel);
    addAndMakeVisible (power);
    power.onStateChange = [this] { repaint(); };
}

void AmpHead::resized()
{
    const int plateY = 132;
    channel.setBounds (92, plateY + 22, 120, 52);
    const int x0 = 226, step = 62;
    for (size_t i = 0; i < knobs.size(); ++i)
        knobs[i].knob->setBounds (x0 + (int) i * step, plateY + 26, 54, 54);
    power.setBounds (getWidth() - 74, plateY + 30, 22, 36);
}

void AmpHead::paint (juce::Graphics& g)
{
    auto r = getLocalBounds().toFloat();
    const bool on = state.getRawParameterValue (ids::ampOn)->load() > 0.5f;

    g.setColour (juce::Colours::black.withAlpha (0.5f));
    g.fillRoundedRectangle (r.translated (0, 6), 10.0f);

    // tolex
    g.setGradientFill (juce::ColourGradient (juce::Colour (0xff2a2a2a), r.getX(), r.getY(), juce::Colour (0xff111111), r.getX(), r.getBottom(), false));
    g.fillRoundedRectangle (r, 10.0f);
    juce::Random rng (7);
    g.setColour (juce::Colours::white.withAlpha (0.025f));
    for (int i = 0; i < 1400; ++i)
        g.fillRect (r.getX() + rng.nextFloat() * r.getWidth(), r.getY() + rng.nextFloat() * r.getHeight(), 1.5f, 1.5f);

    // grill cloth
    auto grill = juce::Rectangle<float> (r.getX() + 18, r.getY() + 16, r.getWidth() - 36, 104);
    g.setColour (juce::Colour (0xff2f2a22));
    g.fillRoundedRectangle (grill, 4.0f);
    g.saveState();
    g.reduceClipRegion (grill.toNearestInt());
    g.setColour (juce::Colour (0xff4b4235).withAlpha (0.55f));
    for (float x = grill.getX() - grill.getHeight(); x < grill.getRight(); x += 5.0f)
    {
        g.drawLine (x, grill.getBottom(), x + grill.getHeight(), grill.getY(), 1.2f);
        g.drawLine (x, grill.getY(), x + grill.getHeight(), grill.getBottom(), 0.6f);
    }
    g.restoreState();
    g.setColour (col::gold.withAlpha (0.7f));
    g.drawRoundedRectangle (grill, 4.0f, 1.5f);

    // logo badge
    auto badge = juce::Rectangle<float> (grill.getCentreX() - 90, grill.getCentreY() - 22, 180, 44);
    g.setColour (juce::Colour (0xff0d0d0d));
    g.fillRoundedRectangle (badge, 6.0f);
    g.setColour (col::gold);
    g.drawRoundedRectangle (badge.reduced (2.0f), 5.0f, 1.0f);
    g.setFont (font (26.0f, true, 0.18f));
    g.drawText ("PB-50", badge.withTrimmedBottom (8.0f), juce::Justification::centred);
    g.setFont (font (8.5f, false, 0.4f));
    g.drawText ("TWIN CHANNEL", badge.withTrimmedTop (30.0f), juce::Justification::centred);

    // gold faceplate
    auto plate = juce::Rectangle<float> (r.getX() + 18, 132, r.getWidth() - 36, 102);
    g.setGradientFill (juce::ColourGradient (juce::Colour (0xffe9cd8a), plate.getX(), plate.getY(), juce::Colour (0xffa9873f), plate.getX(), plate.getBottom(), false));
    g.fillRoundedRectangle (plate, 5.0f);
    g.setColour (juce::Colours::white.withAlpha (0.08f));
    for (float y = plate.getY() + 2; y < plate.getBottom(); y += 2.0f)
        g.drawHorizontalLine ((int) y, plate.getX() + 2, plate.getRight() - 2);
    g.setColour (juce::Colour (0xff5d4a20));
    g.drawRoundedRectangle (plate, 5.0f, 1.0f);

    const auto ink = juce::Colour (0xff1a140a);
    g.setColour (ink);
    g.setFont (font (9.5f, true, 0.14f));
    g.drawText ("INPUT", juce::Rectangle<float> (plate.getX() + 6, plate.getY() + 8, 60, 12), juce::Justification::centred);
    drawJack (g, { plate.getX() + 36, plate.getY() + 50 });

    g.drawText ("CHANNEL", juce::Rectangle<float> (92, plate.getY() + 8, 120, 12), juce::Justification::centred);
    for (size_t i = 0; i < knobs.size(); ++i)
    {
        const auto kb = knobs[i].knob->getBounds().toFloat();
        g.drawText (knobs[i].label, juce::Rectangle<float> (kb.getX() - 8, plate.getY() + 8, kb.getWidth() + 16, 12),
                    juce::Justification::centred);
    }
    g.drawText ("POWER", juce::Rectangle<float> (getWidth() - 94.0f, plate.getY() + 8, 56, 12), juce::Justification::centred);

    // jewel pilot lamp
    drawLed (g, { getWidth() - 38.0f, plate.getY() + 48 }, 8.0f, on);

    g.setFont (font (8.0f, false, 0.3f));
    g.setColour (ink.withAlpha (0.7f));
    g.drawText ("HAND WIRED IN SAO PAULO", plate.withTrimmedTop (plate.getHeight() - 16), juce::Justification::centred);
}

// ---------------------------------------------------------------------------
Cabinet::Cabinet (PboardProcessor& p)
    : proc (p), level (p.apvts, ids::cabLevel, "small"), enable (p.apvts, ids::cabOn)
{
    addAndMakeVisible (level);
    addAndMakeVisible (enable);
    addAndMakeVisible (load);
    addAndMakeVisible (reset);
    enable.onStateChange = [this] { repaint(); };

    load.onClick = [this]
    {
        chooser = std::make_unique<juce::FileChooser> ("Load a cabinet impulse response",
                                                       juce::File::getSpecialLocation (juce::File::userHomeDirectory),
                                                       "*.wav;*.aif;*.aiff");
        chooser->launchAsync (juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles,
                              [this] (const juce::FileChooser& fc)
                              {
                                  const auto f = fc.getResult();
                                  if (f.existsAsFile())
                                      proc.loadImpulseResponse (f);
                              });
    };
    reset.onClick = [this] { proc.loadDefaultImpulseResponse(); };

    irName = proc.getImpulseResponseName();
    startTimerHz (4);
}

void Cabinet::timerCallback()
{
    const auto n = proc.getImpulseResponseName();
    if (n != irName)
    {
        irName = n;
        repaint();
    }
}

void Cabinet::resized()
{
    auto strip = getLocalBounds().removeFromBottom (78).reduced (14, 10);
    enable.setBounds (strip.getX() + 2, strip.getY() + 14, 20, 32);
    level.setBounds (strip.getX() + 32, strip.getY() + 4, 50, 50);
    auto right = strip.withTrimmedLeft (96);
    auto buttons = right.removeFromBottom (24);
    load.setBounds (buttons.removeFromLeft (buttons.getWidth() / 2).reduced (2, 0));
    reset.setBounds (buttons.reduced (2, 0));
}

void Cabinet::paint (juce::Graphics& g)
{
    auto r = getLocalBounds().toFloat();
    const bool on = proc.apvts.getRawParameterValue (ids::cabOn)->load() > 0.5f;

    g.setColour (juce::Colours::black.withAlpha (0.5f));
    g.fillRoundedRectangle (r.translated (0, 6), 10.0f);
    g.setGradientFill (juce::ColourGradient (juce::Colour (0xff262626), r.getX(), r.getY(), juce::Colour (0xff0f0f0f), r.getX(), r.getBottom(), false));
    g.fillRoundedRectangle (r, 10.0f);

    // grill with speaker cones behind it
    auto grill = juce::Rectangle<float> (r.getX() + 16, r.getY() + 16, r.getWidth() - 32, r.getHeight() - 110);
    g.setColour (juce::Colour (0xff1c1a17));
    g.fillRoundedRectangle (grill, 4.0f);
    g.saveState();
    g.reduceClipRegion (grill.toNearestInt());
    const float coneR = juce::jmin (grill.getWidth() * 0.22f, grill.getHeight() * 0.42f);
    for (auto c : { juce::Point<float> (grill.getX() + grill.getWidth() * 0.27f, grill.getCentreY()),
                    juce::Point<float> (grill.getX() + grill.getWidth() * 0.73f, grill.getCentreY()) })
    {
        g.setGradientFill (juce::ColourGradient (juce::Colour (0xff2b2925), c.x, c.y, juce::Colour (0xff141311), c.x + coneR, c.y, true));
        g.fillEllipse (c.x - coneR, c.y - coneR, coneR * 2, coneR * 2);
        g.setColour (juce::Colour (0xff0b0a09));
        g.fillEllipse (c.x - coneR * 0.25f, c.y - coneR * 0.25f, coneR * 0.5f, coneR * 0.5f);
    }
    g.setColour (juce::Colour (0xff8a7f6c).withAlpha (0.16f));
    for (float x = grill.getX(); x < grill.getRight(); x += 3.0f)
        g.drawVerticalLine ((int) x, grill.getY(), grill.getBottom());
    for (float y = grill.getY(); y < grill.getBottom(); y += 3.0f)
        g.drawHorizontalLine ((int) y, grill.getX(), grill.getRight());
    g.restoreState();
    g.setColour (col::gold.withAlpha (0.5f));
    g.drawRoundedRectangle (grill, 4.0f, 1.2f);

    // plate
    g.setColour (col::gold);
    g.setFont (font (11.0f, true, 0.35f));
    g.drawText ("IR CAB", grill.withHeight (22.0f).translated (0, grill.getHeight() - 26), juce::Justification::centred);

    // control strip
    auto strip = r.removeFromBottom (78).reduced (14, 10);
    g.setColour (juce::Colour (0xff070707));
    g.fillRoundedRectangle (strip, 5.0f);
    g.setColour (col::cream.withAlpha (on ? 0.9f : 0.4f));
    g.setFont (font (8.5f, true, 0.12f));
    g.drawText ("ON", juce::Rectangle<float> (strip.getX(), strip.getBottom() - 12, 26, 10), juce::Justification::centred);
    g.drawText ("LEVEL", juce::Rectangle<float> (strip.getX() + 30, strip.getBottom() - 6, 54, 10), juce::Justification::centred);

    auto lcd = strip.withTrimmedLeft (96).removeFromTop (strip.getHeight() - 30).reduced (2.0f, 2.0f);
    g.setColour (juce::Colour (0xff0e1a0c));
    g.fillRoundedRectangle (lcd, 3.0f);
    g.setColour (on ? col::lcd : col::lcd.withAlpha (0.35f));
    g.setFont (juce::Font (juce::FontOptions (juce::Font::getDefaultMonospacedFontName(), 12.0f, juce::Font::plain)));
    g.drawFittedText (irName, lcd.reduced (6.0f, 2.0f).toNearestInt(), juce::Justification::centredLeft, 1);
}

// ---------------------------------------------------------------------------
LevelStrip::LevelStrip (juce::AudioProcessorValueTreeState& s, const juce::String& paramId, juce::String t)
    : knob (s, paramId, "small"), title (std::move (t))
{
    addAndMakeVisible (knob);
}

void LevelStrip::resized()
{
    knob.setBounds (getWidth() / 2 - 26, getHeight() - 66, 52, 52);
}

void LevelStrip::setLevel (float v)
{
    if (std::abs (v - level) > 0.002f)
    {
        level = v;
        repaint (getLocalBounds().withTrimmedBottom (70));
    }
}

void LevelStrip::paint (juce::Graphics& g)
{
    auto r = getLocalBounds().toFloat();
    g.setColour (col::cream.withAlpha (0.85f));
    g.setFont (font (10.0f, true, 0.2f));
    g.drawText (title, r.removeFromTop (18.0f), juce::Justification::centred);

    auto meter = juce::Rectangle<float> (r.getCentreX() - 7, r.getY() + 6, 14, r.getHeight() - 90);
    g.setColour (juce::Colour (0xff050505));
    g.fillRoundedRectangle (meter.expanded (2.0f), 3.0f);

    const float db = juce::Decibels::gainToDecibels (level, -60.0f);
    const float frac = juce::jlimit (0.0f, 1.0f, (db + 60.0f) / 60.0f);
    const int segments = 20;
    const float segH = meter.getHeight() / (float) segments;
    for (int i = 0; i < segments; ++i)
    {
        const float segFrac = (float) (i + 1) / (float) segments;
        auto seg = juce::Rectangle<float> (meter.getX(), meter.getBottom() - segH * (float) (i + 1), meter.getWidth(), segH).reduced (0.0f, 1.0f);
        const auto c = segFrac > 0.95f ? col::ledRed : segFrac > 0.8f ? juce::Colour (0xffffb52e) : juce::Colour (0xff59d36b);
        g.setColour (segFrac <= frac ? c : c.withAlpha (0.12f));
        g.fillRect (seg);
    }
}
} // namespace ui

// ===========================================================================
PboardEditor::PboardEditor (PboardProcessor& p)
    : AudioProcessorEditor (&p), proc (p),
      amp (p.apvts), cab (p),
      gnaw (p.apvts, "GNAW", "DISTORTION", ui::col::gnaw, juce::Colours::white, ids::ratOn,
            { ids::ratDist, ids::ratFilter, ids::ratVol }, { "DISTORTION", "FILTER", "VOLUME" }, "pedal"),
      swirl (p.apvts, "SWIRL", "BBD CHORUS", ui::col::swirl, juce::Colour (0xff10212d), ids::choOn,
             { ids::choRate, ids::choDepth, ids::choMix }, { "RATE", "DEPTH", "MIX" }, "pedalLight"),
      input (p.apvts, ids::inGain, "IN"), output (p.apvts, ids::outLevel, "OUT")
{
    setLookAndFeel (&lnf);

    gnaw.setSelector (std::make_unique<ui::Selector> (p.apvts, ids::ratClip, juce::StringArray { "SI", "LED", "GE" },
                                                      juce::Colours::white, true));

    for (auto* c : std::initializer_list<juce::Component*> { &amp, &cab, &gnaw, &swirl, &input, &output })
        addAndMakeVisible (c);

    setSize (1100, 700);
    startTimerHz (30);

   #if JucePlugin_Build_Standalone
    // JUCE's standalone app mutes the input by default (feedback guard), which
    // silences a guitar app. Unmute, unless the input is the built-in mic.
    if (p.wrapperType == juce::AudioProcessor::wrapperType_Standalone)
        if (auto* holder = juce::StandalonePluginHolder::getInstance())
        {
            auto* device = holder->deviceManager.getCurrentAudioDevice();
            const bool builtInMic = device != nullptr && device->getName().containsIgnoreCase ("microphone");
            if (! builtInMic)
                holder->getMuteInputValue().setValue (false);
        }
   #endif
}

PboardEditor::~PboardEditor()
{
    setLookAndFeel (nullptr);
}

void PboardEditor::timerCallback()
{
    const float in = proc.inputPeak.exchange (0.0f), out = proc.outputPeak.exchange (0.0f);
    inLevel = juce::jmax (in, inLevel * 0.85f);
    outLevel = juce::jmax (out, outLevel * 0.85f);
    input.setLevel (inLevel);
    output.setLevel (outLevel);
}

void PboardEditor::resized()
{
    amp.setBounds (30, 24, 690, 250);
    cab.setBounds (740, 24, 330, 250);
    swirl.setBounds (250, 336, 230, 320);
    gnaw.setBounds (530, 336, 230, 320);
    input.setBounds (832, 360, 100, 270);
    output.setBounds (950, 360, 100, 270);
}

void PboardEditor::paint (juce::Graphics& g)
{
    const auto b = getLocalBounds().toFloat();
    g.setGradientFill (juce::ColourGradient (juce::Colour (0xff23252a), b.getCentreX(), 0.0f,
                                             ui::col::room, b.getCentreX(), b.getBottom(), false));
    g.fillAll();

    // pedalboard
    auto board = juce::Rectangle<float> (20, 304, 1060, 380);
    g.setColour (juce::Colours::black.withAlpha (0.6f));
    g.fillRoundedRectangle (board.translated (0, 8), 8.0f);
    g.setColour (ui::col::boardEdge);
    g.fillRoundedRectangle (board, 8.0f);
    auto surface = board.reduced (8.0f);
    g.setColour (ui::col::board);
    g.fillRoundedRectangle (surface, 5.0f);
    g.setColour (juce::Colours::black.withAlpha (0.25f));
    for (float y = surface.getY() + 4; y < surface.getBottom(); y += 4.0f)
        g.drawHorizontalLine ((int) y, surface.getX() + 2, surface.getRight() - 2);

    // brand plate on the empty corner of the board
    auto plate = juce::Rectangle<float> (44, 600, 160, 54);
    g.setColour (juce::Colour (0xff0c0c0c));
    g.fillRoundedRectangle (plate, 5.0f);
    g.setColour (ui::col::gold);
    g.drawRoundedRectangle (plate.reduced (2.0f), 4.0f, 1.0f);
    g.setFont (ui::font (24.0f, true, 0.1f));
    g.drawText ("pboard", plate.withTrimmedBottom (12), juce::Justification::centred);
    g.setFont (ui::font (8.0f, false, 0.4f));
    g.drawText ("by TOFU", plate.withTrimmedTop (34), juce::Justification::centred);

    // audio interface box behind the IN / OUT strips
    auto io = juce::Rectangle<float> (818, 344, 246, 300);
    g.setGradientFill (juce::ColourGradient (juce::Colour (0xff35383d), io.getX(), io.getY(), juce::Colour (0xff1c1e21), io.getX(), io.getBottom(), false));
    g.fillRoundedRectangle (io, 10.0f);
    g.setColour (juce::Colour (0xff0b0c0d));
    g.drawRoundedRectangle (io, 10.0f, 1.5f);
    g.setColour (ui::col::cream.withAlpha (0.4f));
    g.setFont (ui::font (8.5f, true, 0.3f));
    g.drawText ("I / O", io.removeFromBottom (14.0f), juce::Justification::centred);
}

void PboardEditor::paintOverChildren (juce::Graphics& g)
{
    // patch cables: interface -> GNAW -> SWIRL -> amp input
    auto cable = [&g] (juce::Point<float> a, juce::Point<float> b, juce::Colour colour, float sag)
    {
        juce::Path p;
        p.startNewSubPath (a);
        const float dx = b.x - a.x;
        p.cubicTo (a.x + dx * 0.35f, a.y + sag, b.x - dx * 0.35f, b.y + sag, b.x, b.y);
        g.setColour (juce::Colours::black.withAlpha (0.45f));
        g.strokePath (p, juce::PathStrokeType (8.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded),
                      juce::AffineTransform::translation (2.0f, 5.0f));
        g.setColour (colour);
        g.strokePath (p, juce::PathStrokeType (6.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
        g.setColour (juce::Colours::white.withAlpha (0.12f));
        g.strokePath (p, juce::PathStrokeType (1.5f), juce::AffineTransform::translation (-1.0f, -1.5f));

        for (auto end : { a, b })
        {
            g.setColour (juce::Colour (0xff9a9a9a));
            g.fillRoundedRectangle (end.x - 6, end.y - 6, 12, 12, 3.0f);
            g.setColour (juce::Colour (0xff3a3a3a));
            g.drawRoundedRectangle (end.x - 6, end.y - 6, 12, 12, 3.0f, 1.0f);
        }
    };

    const auto cableColour = juce::Colour (0xff121212);
    const juce::Point<float> ioOut (818.0f, 456.0f);
    cable (ioOut, gnaw.inputJack(), cableColour, 26.0f);
    cable (gnaw.outputJack(), swirl.inputJack(), cableColour, 26.0f);
    const juce::Point<float> ampIn ((float) amp.getX() + 54.0f, (float) amp.getY() + 182.0f);
    cable (swirl.outputJack(), ampIn, juce::Colour (0xff1a1410), 120.0f);
}
