﻿#pragma once
#include "Devices/Elliptical/ellipticaltestdata.h"
#include "devices/domyoselliptical/domyoselliptical.h"

class DomyosEllipticalTestData : public EllipticalTestData {

public:
    DomyosEllipticalTestData() : EllipticalTestData("Domyos Elliptical") {
        this->addDeviceName("Domyos-EL", comparison::StartsWith);
        this->addInvalidDeviceName("DomyosBridge",comparison::StartsWith);
    }


    deviceType get_expectedDeviceType() const override { return deviceType::DomyosElliptical; }

    bool get_isExpectedDevice(bluetoothdevice * detectedDevice) const override {
        return dynamic_cast<domyoselliptical*>(detectedDevice)!=nullptr;
    }
};

