/*
PresenceActivityType.java
Copyright (C) 2010-2013  Belledonne Communications, Grenoble, France

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
*/

package org.linphone.core;

/** Activities as defined in section 3.2 of RFC 4480 */
public enum PresenceActivityType {
	/** This value is not defined in the RFC, it corresponds to no activity with a basic status of "closed". */
	Offline(0),
	/** This value is not defined in the RFC, it corresponds to no activity with a basic status of "open". */
	Online(1),
	/** The person has a calendar appointment, without specifying exactly of what type. This activity is
		*  indicated if more detailed information is not available or the person chooses not to reveal more
		* information. */
	Appointment(2),
	/** The person is physically away from all interactive communication devices. */
	Away(3),
	/** The person is eating the first meal of the day, usually eaten in the morning. */
	Breakfast(4),
	/** The person is busy, without further details. */
	Busy(5),
	/** The person is having his or her main meal of the day, eaten in the evening or at midday. */
	Dinner(6),
	/**  This is a scheduled national or local holiday. */
	Holiday(7),
	/** The person is riding in a vehicle, such as a car, but not steering. */
	InTransit(8),
	/** The person is looking for (paid) work. */
	LookingForWork(9),
	/** The person is eating his or her midday meal. */
	Lunch(10),
	/** The person is scheduled for a meal, without specifying whether it is breakfast, lunch, or dinner,
		*  or some other meal. */
	Meal(11),
	/** The person is in an assembly or gathering of people, as for a business, social, or religious purpose.
		*  A meeting is a sub-class of an appointment. */
	Meeting(12),
	/** The person is talking on the telephone. */
	OnThePhone(13),
	/** The person is engaged in an activity with no defined representation. A string describing the activity
		*  in plain text SHOULD be provided. */
	Other(14),
	/** A performance is a sub-class of an appointment and includes musical, theatrical, and cinematic
		*  performances as well as lectures. It is distinguished from a meeting by the fact that the person
		*  may either be lecturing or be in the audience, with a potentially large number of other people,
		*  making interruptions particularly noticeable. */
	Performance(15),
	/** The person will not return for the foreseeable future, e.g., because it is no longer working for
		*  the company. */
	PermanentAbsence(16),
	/** The person is occupying himself or herself in amusement, sport, or other recreation. */
	Playing(17),
	/** The person is giving a presentation, lecture, or participating in a formal round-table discussion. */
	Presentation(18),
	/** The person is visiting stores in search of goods or services. */
	Shopping(19),
	/** The person is sleeping.*/
	Sleeping(20),
	/** The person is observing an event, such as a sports event. */
	Spectator(21),
	/** The person is controlling a vehicle, watercraft, or plane. */
	Steering(22),
	/** The person is on a business or personal trip, but not necessarily in-transit. */
	Travel(23),
	/** The person is watching television. */
	TV(24),
	/** The activity of the person is unknown. */
	Unknown(25),
	/** A period of time devoted to pleasure, rest, or relaxation. */
	Vacation(26),
	/** The person is engaged in, typically paid, labor, as part of a profession or job. */
	Working(27),
	/** The person is participating in religious rites. */
	Worship(28),
	Invalid(29);

	protected final int mValue;

	private PresenceActivityType(int value) {
		mValue = value;
	}

	public int toInt() {
		return mValue;
	}

	static protected PresenceActivityType fromInt(int value) {
		switch (value) {
		case 0: return Offline;
		case 1: return Online;
		case 2: return Appointment;
		case 3: return Away;
		case 4: return Breakfast;
		case 5: return Busy;
		case 6: return Dinner;
		case 7: return Holiday;
		case 8: return InTransit;
		case 9: return LookingForWork;
		case 10: return Lunch;
		case 11: return Meal;
		case 12: return Meeting;
		case 13: return OnThePhone;
		case 14: return Other;
		case 15: return Performance;
		case 16: return PermanentAbsence;
		case 17: return Playing;
		case 18: return Presentation;
		case 19: return Shopping;
		case 20: return Sleeping;
		case 21: return Spectator;
		case 22: return Steering;
		case 23: return Travel;
		case 24: return TV;
		case 25: return Unknown;
		case 26: return Vacation;
		case 27: return Working;
		case 28: return Worship;
		default: return Invalid;
		}
	}
}
