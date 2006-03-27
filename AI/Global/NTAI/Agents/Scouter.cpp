#include "../helper.h"

// ||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||
void Scouter::InitAI(Global* GLI){
	G = GLI;
	CSunParser MP(G);
	MP.LoadFile(string("maps\\")+string(G->cb->GetMapName()).substr(0,string(G->cb->GetMapName()).find('.'))+".smd");
    int NumStartLocations = 0;
    float3 StartLoc[MAX_TEAMS];
    for(int n=0; n<MAX_TEAMS; n++){
    	char c[8];
    	//itoa(n,c,10);// something to do with ANSI standards and 64 bit compatability
		sprintf(c,"%i",n);
    	MP.GetDef(StartLoc[n].x, "-1", string("MAP\\TEAM") + string(c) + "\\StartPosX");
    	MP.GetDef(StartLoc[n].z, "-1", string("MAP\\TEAM") + string(c) + "\\StartPosZ");
    	if(StartLoc[n].x >= 0 && StartLoc[n].z >= 0) NumStartLocations++;
    }
	for(int i = 0; i< NumStartLocations; i++){
		start_pos.push_back(StartLoc[i]);
	}
	for(vector<float3>::iterator kj = G->M->hotspot.begin(); kj != G->M->hotspot.end(); ++kj){
		mexes.push_back(*kj);
	}
}

// ||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||

void Scouter::UnitFinished(int unit){
	NLOG("Scouter::UnitFinished");
	const UnitDef* ud = G->cb->GetUnitDef(unit);
	if( ud == 0) return;
	bool naset = false;
	if(G->dynamic_selection == false){
		vector<string> v;
		v = bds::set_cont(v,G->Get_mod_tdf()->SGetValueMSG("AI\\Scouters"));
		if(v.empty() == false){
			for(vector<string>::iterator vi = v.begin(); vi != v.end(); ++vi){
				if(*vi == ud->name){
					naset = true;
					break;
				}
			}
		}
	}else{
		// determine if this is a scouter or not dynamically
		if((ud->speed > G->scout_speed)&&(ud->movedata != 0)&&(ud->canfly == false)&&(ud->transportCapacity == 0)&&(ud->isCommander == false)&&(ud->builder == false)) naset = true;
		if((ud->weapons.empty() == true)&&(ud->canfly == true)&&(ud->transportCapacity == 0)&&(ud->isCommander == false)&&(ud->builder == false)) naset = true;
	}
	float3 dpos = G->cb->GetUnitPos(unit);
	if((ud->extractsMetal >0)&&(cp.empty() == false)){
		for(map<int, list<float3> >::iterator i = cp.begin(); i != cp.end(); ++i){
			if(i->second.empty() == false){
				for(list<float3>::iterator j = i->second.begin(); j != i->second.end(); ++j){
					if(*j == dpos){
						i->second.erase(j);
						break;
					}
				}
			}
		}
		if(mexes.empty() == false){
			for(list<float3>::iterator i = mexes.begin(); i != mexes.end(); ++i){
				if( *i == dpos){
					mexes.erase(i);
					break;
				}
			}
		}
	}
	if(naset){
		if((G->cb->GetCurrentFrame()%3 == 2)||(cp.empty() == true)){
            cp[unit] = start_pos;
		}else if(G->cb->GetCurrentFrame()%3 == 1){
			if(this->sectors.empty() == true){
				list<float3> cf;
				for(int xa = 0; xa < G->Ch->xSectors; xa++){
					for(int ya = 0; ya < G->Ch->ySectors; ya++){
						if(xa > ya){
							sectors.push_back(G->Ch->sector[xa][ya].centre);
						}else{
							sectors.insert(cf.begin(),G->Ch->sector[xa][ya].centre);
						}
					}
				}
				cp[unit] = cf;
			}else{
				cp[unit] = sectors;
			}
		}else if(G->cb->GetCurrentFrame()%3 == 0){
			if(G->mexscouting == true){
				cp[unit] = mexes;
			}else{
				if(this->sectors.empty() == true){
					list<float3> cf;
					for(int xa = 0; xa < G->Ch->xSectors; xa++){
						for(int ya = 0; ya < G->Ch->ySectors; ya++){
							if(xa > ya){
								sectors.push_back(G->Ch->sector[xa][ya].centre);
							}else{
								sectors.insert(cf.begin(),G->Ch->sector[xa][ya].centre);
							}
						}
					}
					cp[unit] = cf;
				}else{
					cp[unit] = sectors;
				}
			}
		}
		G->L.print("scouter found");
	 }
}

// ||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||

void Scouter::UnitMoveFailed(int unit){
	NLOG("Scouter::UnitMoveFailed");
	UnitIdle(unit);
}

// ||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||

void Scouter::UnitIdle(int unit){
	NLOG("Scouter::UnitIdle");
	if(G->cb->GetUnitDef(unit) == 0) return;
	bool naset = false;
	const UnitDef* ud = G->cb->GetUnitDef(unit);
	if(G->dynamic_selection == false){
		vector<string> v;
		v = bds::set_cont(v,G->Get_mod_tdf()->SGetValueMSG("AI\\Scouters"));
		if(v.empty() == false){
			for(vector<string>::iterator vi = v.begin(); vi != v.end(); ++vi){
				if(*vi == ud->name){
					naset = true;
					break;
				}
			}
		}
	}else{
		// determine if this si a scouter or not dynamically
		if((ud->speed > G->scout_speed)&&(ud->movedata != 0)&&(ud->canfly == false)&&(ud->transportCapacity == 0)&&(ud->isCommander == false)&&(ud->builder == false)) naset = true;
		if((ud->weapons.empty() == true)&&(ud->canfly == true)&&(ud->transportCapacity == 0)&&(ud->isCommander == false)&&(ud->builder == false)) naset = true;
	}
	
	if(naset){
		TCommand TCS(tc,_T("scouter::unitIdle"));
		tc.unit = unit;
		tc.created = G->cb->GetCurrentFrame();
		tc.Priority = tc_instant;
		for(int i = G->cb->GetCurrentFrame(); i%46 != 0; i++){
			cp[unit].push_back(cp[unit].front());
			cp[unit].pop_front();
		}
		float3 pos_temp = cp[unit].front();
		tc.c.params.push_back(pos_temp.x);
		tc.c.params.push_back(G->cb->GetElevation(pos_temp.x,pos_temp.z));
		tc.c.params.push_back(pos_temp.z);
		if(tc.c.params.empty() == true){
			return;
		}
		//tc.c.params.push_back(40);
		tc.c.timeOut = 6 MINUTES + G->cb->GetCurrentFrame();
		tc.created = G->cb->GetCurrentFrame();
		tc.c.id=CMD_MOVE;
		cp[unit].push_back(cp[unit].front());
		cp[unit].erase(cp[unit].begin());
		G->GiveOrder(tc);
	 }
}

// ||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||

void Scouter::UnitDestroyed(int unit){
	NLOG("Scouter::UnitDestroyed");
	if(cp.empty() == false){
		for(map< int, list<float3> >::iterator q = cp.begin(); q != cp.end();++q){
			if(q->first == unit){
				cp.erase(q);
				break;
			}
		}
		const UnitDef* ud = G->cb->GetUnitDef(unit);
		if(ud == 0){
			return;
		}else{
			float3 dpos = G->cb->GetUnitPos(unit);
			if((ud->extractsMetal >0)&&(cp.empty() == false)){
				for(map<int, list<float3> >::iterator i = cp.begin(); i != cp.end(); ++i){
					i->second.push_back(dpos);
				}
				mexes.push_back(dpos);
			}
		}
	}
}

float3 Scouter::GetMexScout(int i){
	NLOG("Scouter::GetMexScout");
	int j=0;
	int h = (int)mexes.size();
	if(i > h-2){
		j = (h-2)%i ;
		if(j==0) j++;
	}else{
		j = i;
		if(j==0) j++;
	}
	list<float3>::iterator ij= mexes.begin();
	for(int hj = 0; hj != j; ++hj){
		++ij;
	}
	return *ij;
}

float3 Scouter::GetStartPos(int i){
	NLOG("Scouter::GetStartPos");
	int j;
	if(i > (int)start_pos.size()){
		j = (int)start_pos.size()%i;
	}else{
		j = i;
	}
	list<float3>::iterator ij= start_pos.begin();
	for(int hj = 0; hj != j; ++hj){
		++ij;
	}
	return *ij;
}
// ||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||

