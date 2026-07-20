# Transient

Make a single point light the transient light

In path state keep track of the rollingRayT

lightDistanceSinceStart = speedOfLight * timeSincePulseStart
lightDistanceDuringPulse = speedOfLight * pulseDuration

OnSwitch Mode:
transientFactor = (lightDistanceSinceStart - rollingRayT) >= 0

Pulse Mode:
transientFactor = rollingRayT >= lightDistanceSinceStart && rollingRayT < lightDistanceSinceStart + lightDistanceDuringPulse

Gauss Pulse Mode:
halfRange = lightDistanceDuringPulse * 0.5
mid = lightDistanceSinceStart + halfRange
t = abs(rollingRayT - mid) / halfRange
transientFactor = smoothstep(0, 1, t)

radiance *= transientFactor

## IOR

TODO