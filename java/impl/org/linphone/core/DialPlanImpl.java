package org.linphone.core;


public class DialPlanImpl implements DialPlan {
    private final String countryName;
    private final String countryCode;
    private final String countryCallingCode;
    private final int numberLength;
    private final String usualPrefix;

    public DialPlanImpl(String countryName, String countryCode, String countryCallingCode, int numberLength, String usualPrefix) {
        this.countryName = countryName;
        this.countryCode = countryCode;
        this.countryCallingCode = countryCallingCode;
        this.numberLength = numberLength;
        this.usualPrefix = usualPrefix;
    }


    @Override
    public final String getCountryCode() {
        return countryCode;
    }

    @Override
    public final String getCountryName() {
        return countryName;
    }

    @Override
    public final String getCountryCallingCode() {
        return countryCallingCode;
    }

    @Override
    public final int getNumberLength() {
        return numberLength;
    }

    @Override
    public final String getUsualPrefix() {
        return usualPrefix;
    }
}
