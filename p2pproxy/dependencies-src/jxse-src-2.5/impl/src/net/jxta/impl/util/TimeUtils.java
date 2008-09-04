/*
 * Copyright (c) 2002-2007 Sun Microsystems, Inc.  All rights reserved.
 *  
 *  The Sun Project JXTA(TM) Software License
 *  
 *  Redistribution and use in source and binary forms, with or without 
 *  modification, are permitted provided that the following conditions are met:
 *  
 *  1. Redistributions of source code must retain the above copyright notice,
 *     this list of conditions and the following disclaimer.
 *  
 *  2. Redistributions in binary form must reproduce the above copyright notice, 
 *     this list of conditions and the following disclaimer in the documentation 
 *     and/or other materials provided with the distribution.
 *  
 *  3. The end-user documentation included with the redistribution, if any, must 
 *     include the following acknowledgment: "This product includes software 
 *     developed by Sun Microsystems, Inc. for JXTA(TM) technology." 
 *     Alternately, this acknowledgment may appear in the software itself, if 
 *     and wherever such third-party acknowledgments normally appear.
 *  
 *  4. The names "Sun", "Sun Microsystems, Inc.", "JXTA" and "Project JXTA" must 
 *     not be used to endorse or promote products derived from this software 
 *     without prior written permission. For written permission, please contact 
 *     Project JXTA at http://www.jxta.org.
 *  
 *  5. Products derived from this software may not be called "JXTA", nor may 
 *     "JXTA" appear in their name, without prior written permission of Sun.
 *  
 *  THIS SOFTWARE IS PROVIDED ``AS IS'' AND ANY EXPRESSED OR IMPLIED WARRANTIES,
 *  INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND 
 *  FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL SUN 
 *  MICROSYSTEMS OR ITS CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, 
 *  INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT 
 *  LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, 
 *  OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF 
 *  LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING 
 *  NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, 
 *  EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *  
 *  JXTA is a registered trademark of Sun Microsystems, Inc. in the United 
 *  States and other countries.
 *  
 *  Please see the license information page at :
 *  <http://www.jxta.org/project/www/license.html> for instructions on use of 
 *  the license in source files.
 *  
 *  ====================================================================
 *  
 *  This software consists of voluntary contributions made by many individuals 
 *  on behalf of Project JXTA. For more information on Project JXTA, please see 
 *  http://www.jxta.org.
 *  
 *  This license is based on the BSD license adopted by the Apache Foundation. 
 */
package net.jxta.impl.util;

/**
 * Utilities for manipulating absolute and relative times and for accelerating
 * and decelerating time for testing purposes.
 * <p/>
 * The "time warp" functionality is useful for debugging and is scoped to
 * the class loader in which the TimeUtils class is loaded.
 */
public final class TimeUtils {

    /**
     * Zero milliseconds (yes its redundant).
     */
    public static final long ZEROMILLISECONDS = 0L;

    /**
     * The number of milliseconds in a millisecond. (yes its redundant).
     */
    public static final long AMILLISECOND = 1L;

    /**
     * The number of milliseconds in a hundredth of a second.
     */
    public static final long AHUNDREDTHOFASECOND = 10 * AMILLISECOND;

    /**
     * The number of milliseconds in a tenth of a second.
     */
    public static final long ATENTHOFASECOND = 100 * AMILLISECOND;

    /**
     * The number of milliseconds in a second.
     */
    public static final long ASECOND = 1000 * AMILLISECOND;

    /**
     * The number of milliseconds in a minute.
     */
    public static final long AMINUTE = 60 * ASECOND;

    /**
     * The number of milliseconds in an hour.
     */
    public static final long ANHOUR = 60 * AMINUTE;

    /**
     * The number of milliseconds in a day.
     */
    public static final long ADAY = 24 * ANHOUR;

    /**
     * The number of milliseconds in a week.
     */
    public static final long AWEEK = 7 * ADAY;

    /**
     * The number of milliseconds in a fortnight (two weeks).
     */
    public static final long AFORTNIGHT = 14 * ADAY;

    /**
     * The number of milliseconds in the month of January.
     */
    public static final long AJANUARY = 31 * ADAY;

    /**
     * The number of milliseconds in the month of February in a non-leap year.
     */
    public static final long AFEBRUARY = 28 * ADAY;

    /**
     * The number of milliseconds in the month of February in a leap year.
     */
    public static final long ALEAPFEBRUARY = 29 * ADAY;

    /**
     * The number of milliseconds in the month of March.
     */
    public static final long AMARCH = 31 * ADAY;

    /**
     * The number of milliseconds in the month of April.
     */
    public static final long ANAPRIL = 30 * ADAY;

    /**
     * The number of milliseconds in the month of May.
     */
    public static final long AMAY = 31 * ADAY;

    /**
     * The number of milliseconds in the month of June.
     */
    public static final long AJUNE = 30 * ADAY;

    /**
     * The number of milliseconds in the month of July.
     */
    public static final long AJULY = 31 * ADAY;

    /**
     * The number of milliseconds in the month of August.
     */
    public static final long ANAUGUST = 31 * ADAY;

    /**
     * The number of milliseconds in the month of September.
     */
    public static final long ASEPTEMBER = 30 * ADAY;

    /**
     * The number of milliseconds in the month of October.
     */
    public static final long ANOCTOBER = 31 * ADAY;

    /**
     * The number of milliseconds in the month of November.
     */
    public static final long ANOVEMBER = 30 * ADAY;

    /**
     * The number of milliseconds in the month of December.
     */
    public static final long ADECEMBER = 31 * ADAY;

    /**
     * The number of milliseconds in a non-leap year.
     */
    public static final long AYEAR = AJANUARY + AFEBRUARY + AMARCH + ANAPRIL + AMAY + AJUNE + AJULY + ANAUGUST + ASEPTEMBER
            + ANOCTOBER + ANOVEMBER + ADECEMBER;

    /**
     * The number of milliseconds in a leap year.
     */
    public static final long ALEAPYEAR = AJANUARY + ALEAPFEBRUARY + AMARCH + ANAPRIL + AMAY + AJUNE + AJULY + ANAUGUST
            + ASEPTEMBER + ANOCTOBER + ANOVEMBER + ADECEMBER;

    /**
     * This odd little guy is for use in testing. it is applied anywhere the
     * current time is used and allows modules which use timeutils to be tested
     * through long (simulated) periods of time passing.
     */
    static volatile long TIMEWARP = 0;

    /**
     * Absolute time in millis at which we began timewarping.
     */
    static long WARPBEGAN = 0;

    /**
     * The rate at which time is warped using the auto-warper.
     */
    static double WARPFACTOR = 1.0;

    /**
     * Don't let anyone instantiate this class.
     */
    private TimeUtils() {
    }

    /**
     * Return the current time. This value may differ from the value returned
     * by {@link System#currentTimeMillis()} if the {@link #timeWarp(long)} or
     * {@link #autoWarp(double)} features are being used. Using
     * {@link #timeNow()} allows test harnesses to simulate long periods of
     * time passing.
     *
     * @return The current time which has been possibly adjusted by a "time warp"
     *         factor.
     */
    public static long timeNow() {
        long now = System.currentTimeMillis();

        if (WARPFACTOR != 1.0) {
            long elapsed = now - WARPBEGAN;

            long dialation = (long) (elapsed * WARPFACTOR);

            TIMEWARP += (dialation - elapsed);
        }

        return now + TIMEWARP;
    }

    /**
     * Convert a duration into a duration expressed in milliseconds to absolute
     * time realtive to the current real time. Special handling for the maximum
     * and minimum durations converts them to the maximum and minimum absolute
     * times.
     *
     * @param duration a time duration expressed in milliseconds.
     * @return an absolute time in milliseconds based on the duration's
     *         relation to the current real time.
     */
    public static long toAbsoluteTimeMillis(long duration) {

        return toAbsoluteTimeMillis(duration, timeNow());
    }

    /**
     * Convert a duration into a duration expressed in milliseconds to absolute
     * time realtive to the provided absolute time. Special handling for the
     * maximum and minimum durations converts them to the maximum and minimum
     * absolute times.
     *
     * @param duration a time duration expressed in milliseconds.
     * @param fromWhen an absolute time expressed in milliseconds.
     * @return an absolute time in milliseconds based on the duration's
     *         relation to the provided absolute time.
     */
    public static long toAbsoluteTimeMillis(long duration, long fromWhen) {

        // Special cases for the boundaries.

        if (Long.MAX_VALUE == duration) {
            return Long.MAX_VALUE;
        }

        if (Long.MIN_VALUE == duration) {
            return Long.MIN_VALUE;
        }

        long whence = fromWhen + duration;

        if (duration > 0) {
            // check for overflow
            if (whence < fromWhen) {
                whence = Long.MAX_VALUE;
            }

        } else {
            // check for underflow
            if (whence > fromWhen) {
                whence = Long.MIN_VALUE;
            }
        }

        return whence;
    }

    /**
     * Convert an absolute real time in milliseconds to a duration relative
     * to the current real time. Special handling for the maximum and minimum
     * absolute times converts them to the maximum and minimum durations.
     *
     * @param whence an absolute real time expressed in milliseconds.
     * @return a duration expressed in milliseconds relative to the current
     *         real time.
     */
    public static long toRelativeTimeMillis(long whence) {
        return toRelativeTimeMillis(whence, timeNow());
    }

    /**
     * Convert an absolute real time in milliseconds to a duration relative
     * to the specified absolute real time. Special handling for the maximum
     * and minimum absolute times converts them to the maximum and minimum
     * durations.
     *
     * @param whence   An absolute real time expressed in milliseconds.
     * @param fromWhen The base time in absolute real time expressed in
     *                 milliseconds from which the relative time will be calculated.
     * @return a duration expressed in milliseconds relative to the provided
     *         absolute time.
     */
    public static long toRelativeTimeMillis(long whence, long fromWhen) {
        // Special cases for the boundaries.

        if (Long.MAX_VALUE == whence) {
            return Long.MAX_VALUE;
        }

        if (Long.MIN_VALUE == whence) {
            return Long.MIN_VALUE;
        }

        return whence - fromWhen;
    }

    /**
     * Multiplies a duration in relative milliseconds by a multiplier while
     * accounting for overflow and underflow of the magnitude value.
     *
     * @param duration   a time duration expressed in milliseconds.
     * @param multiplier a non-negative value which will be multiplied against
     *                   the duration.
     * @return a time duration expressed in milliseconds.
     */
    public static long multiplyRelativeTimeMillis(long duration, long multiplier) {
        if (multiplier < 0) {
            throw new IllegalArgumentException("Only non-negative multipliers are allowed.");
        }

        long result_mag = (Long.MIN_VALUE != duration)
                ? Long.highestOneBit(Math.abs(duration)) + Long.highestOneBit(multiplier)
                : Long.SIZE + Long.highestOneBit(multiplier);
        long result = duration * multiplier;

        if (result_mag > (Long.SIZE - 1)) {
            // over or underflowed
            result = (duration < 0) ? Long.MIN_VALUE : Long.MAX_VALUE;
        }

        return result;
    }

    /**
     * A utility for advancing the timewarp by the number of milliseconds
     * specified. May not be used if you are also using {@link #autoWarp(double)}.
     *
     * @param advanceby Advance the timewarp by the number of milliseconds
     *                  specified.
     */
    public static void timeWarp(long advanceby) {
        if (0 != WARPBEGAN) {
            throw new IllegalStateException("auto time warping already initialized at warp factor " + WARPFACTOR);
        }

        TIMEWARP += advanceby;
    }

    /**
     * A utility for automagically adjusting the time dialation of the
     * time warp.
     *
     * @param warpfactor a decimal ratio at which artifical time will pass.
     *                   <p/>
     *                   <ul>
     *                   <li>To have time pass at the rate of an hour every minute, initialize
     *                   with: <tt>(double)(TimeUtils.ANHOUR / TimeUtils.AMINUTE)</tt>.</li>
     *                   <li>To have time pass at five times normal rate, initialize
     *                   with: <tt>5.0</tt>.</li>
     *                   <li>etc.</li>
     *                   </ul>
     */
    public static void autoWarp(double warpfactor) {
        if (0 != WARPBEGAN) {
            throw new IllegalStateException("Auto time warping already initialized at warp factor " + WARPFACTOR);
        }

        if (warpfactor <= 0.0) {
            throw new IllegalArgumentException("Time should not stand still or run backwards. It's unnatural.");
        }

        if (warpfactor != 1.0) {
            WARPFACTOR = warpfactor;
            WARPBEGAN = System.currentTimeMillis();
        }
    }

    /**
     * Return a relative time adjusted by the current warping factor.
     * Needed for external methods which do not use {@link #timeNow()} such
     * as {@link Object#wait(long)} and
     * {@link java.net.Socket#setSoTimeout(int)}.
     *
     * @param initial The initial time value to "warp".
     * @return The provided initial time adjusted by the current warping factor.
     */
    public static long warpedRelativeTime(long initial) {
        if (0 == initial) {
            return 0;
        }

        long adjusted = (long) (initial * WARPFACTOR);

        // Handle overflow and underflow
        if (initial < 0) {
            if (adjusted >= 0) {
                adjusted = Long.MIN_VALUE;
            }
        } else {
            if (adjusted < 0) {
                adjusted = Long.MAX_VALUE;
            }
        }

        // since 0 is usually a special "wait forever" value we don't allow time
        // to be reduced to zero because that would cause a change of behaviour
        // in many cases.
        return (0 != adjusted) ? adjusted : 1;
    }
}
