# Metasound Dattorro Reverb Implementation

### This repository is my implementation of Dattorro Reverb in Unreal Engine 5 through creation of a metasound node

Link to the paper and algorithm: https://ccrma.stanford.edu/~dattorro/EffectDesignPart1.pdf

#### The Basic Components are as follows:

- All-Pass Filter [https://thewolfsound.com/allpass-filter/] - A filter that passes all frequencies through equally in gain but changes the phase relations among various frequencies.
- Low-Pass Filter [https://www.youtube.com/watch?v=lagfhNjMuQM] - A filter which allows signals that have a lower frequency than their selected cutoff frequency pass through. This filtered signal is then attenuated with frequencies higher than the cutoff freuency.
- Delay Lines [https://docs.juce.com/master/tutorial_dsp_delay_line.html#tutorial_dsp_delay_line_what_is_delay_line] - A fundamental tool in digital signal processing. It simply allows for a signal to be delayed by a number of samples, using multiple delay lines and summing these signals back together at different intervals can create many fun effects.

Perhaps try to implement positions into the node for reverb
