/*  ------------------------------------------------------------------
    Copyright (c) 2011-2020 Marc Toussaint
    email: toussaint@tu-berlin.de

    This code is distributed under the MIT License.
    Please see <root-path>/LICENSE for details.
    --------------------------------------------------------------  */

#include "F_static.h"
#include "forceExchange.h"

F_netForce::F_netForce(int iShape, bool _transOnly, bool _zeroGravity) : i(iShape), transOnly(_transOnly) {
  order=0;
  if(_zeroGravity) {
    gravity = 0.;
  } else {
    gravity = rai::getParameter<double>("F_static/gravity", 9.81);
  }
}

void F_netForce::phi(arr& y, arr& J, const rai::Configuration& C) {
  rai::Frame* a = C.frames(i);

  arr force = zeros(3);
  arr torque = zeros(3);
  arr Jforce, Jtorque;

  if(gravity) {
    double mass=.1;
    if(a->inertia) mass = a->inertia->mass;
    force(2) += gravity * mass;
  }

  //-- collect contacts and signs FOR ALL shapes attached to this link
  rai::Array<rai::ForceExchange*> contacts;
  arr signs;
  FrameL F;
  F.append(a);
  a->getRigidSubFrames(F);
  for(rai::Frame* f:F) {
    for(rai::ForceExchange* con:f->forces) {
      CHECK(&con->a==f || &con->b==f, "");
      contacts.append(con);
      signs.append((&con->a==f ? +1. : -1.));
    }
  }

#if 0
  for(rai::ForceExchange* con:a->forces) {
    double sign = +1.;
    CHECK(&con->a==a || &con->b==a, "");
    if(&con->b==a) sign=-1.;
#else
  for(uint i=0; i<contacts.N; i++) {
    rai::ForceExchange* con = contacts(i);
    double sign = signs(i);
#endif

    //get the force
    arr f, Jf;
    C.kinematicsContactForce(f, Jf, con);

    //get the POA
    arr poa, Jpoa;
    C.kinematicsContactPOA(poa, Jpoa, con);

    //get object center
    arr p, Jp;
    C.kinematicsPos(p, Jp, a);

    force -= sign * con->force;
    if(!transOnly) torque += sign * crossProduct(poa-p, con->force);

    if(!Jforce.nd){ Jforce = -sign * Jf; }
    else{ Jforce -= sign * Jf; }

    if(!transOnly){
      arr tmp = sign * (skew(poa-p) * Jf - skew(con->force) * (Jpoa-Jp));
      if(!Jtorque.nd) Jtorque = tmp;
      else Jtorque += tmp;
    }
  }


  if(!transOnly){
    y.setBlockVector(force, torque);
    if(Jforce.N) J.setBlockMatrix(Jforce, Jtorque);
    else J.resize(6, C.q.N).setZero();
  }else{
    y=force;
    J=Jforce;
  }
}

uint F_netForce::dim_phi(const rai::Configuration& K) {
  if(transOnly) return 3;
  return 6;
}

