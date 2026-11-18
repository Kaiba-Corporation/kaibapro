--The Winged Dragon of Ra
EFFECT_FLAG_COPY_INHERIT=EFFECT_FLAG_COPY_INHERIT or 0 --Bot Theater: not in this engine
function c700000002.initial_effect(c)
	--Summon with 3 Tribute
	local e1=Effect.CreateEffect(c)
	e1:SetProperty(EFFECT_FLAG_CANNOT_DISABLE+EFFECT_FLAG_UNCOPYABLE)
	e1:SetType(EFFECT_TYPE_SINGLE)
	e1:SetCode(EFFECT_LIMIT_SUMMON_PROC)
	e1:SetCondition(c700000002.sumoncon)
	e1:SetOperation(c700000002.sumonop)
	e1:SetValue(SUMMON_TYPE_ADVANCE)
	c:RegisterEffect(e1)
	local e2=Effect.CreateEffect(c)
	e2:SetType(EFFECT_TYPE_SINGLE)
	e2:SetCode(EFFECT_LIMIT_SET_PROC)
	e2:SetCondition(c700000002.setcon)
	c:RegisterEffect(e2)
	--Summon Cannot be Negated
	local e3=Effect.CreateEffect(c)
	e3:SetType(EFFECT_TYPE_SINGLE)
	e3:SetCode(EFFECT_CANNOT_DISABLE_SUMMON)
	e3:SetProperty(EFFECT_FLAG_CANNOT_DISABLE+EFFECT_FLAG_UNCOPYABLE)
	c:RegisterEffect(e3)
	--summon success
	local e4=Effect.CreateEffect(c)
	e4:SetType(EFFECT_TYPE_SINGLE+EFFECT_TYPE_CONTINUOUS)
	e4:SetCode(EVENT_SUMMON_SUCCESS)
	e4:SetOperation(c700000002.sumsuc)
	c:RegisterEffect(e4)
	--resurrection
	local e5=Effect.CreateEffect(c)
	e5:SetType(EFFECT_TYPE_SINGLE+EFFECT_TYPE_CONTINUOUS)
	e5:SetCountLimit(1)
	e5:SetCode(EVENT_SPSUMMON_SUCCESS)
	e5:SetProperty(EFFECT_FLAG_DAMAGE_STEP+EFFECT_FLAG_DELAY)
	e5:SetCondition(c700000002.egpcon)
	e5:SetTarget(c700000002.immortal)
	c:RegisterEffect(e5)
	--unstoppable attack
	local e6=Effect.CreateEffect(c)
	e6:SetType(EFFECT_TYPE_SINGLE)
	e6:SetCode(EFFECT_UNSTOPPABLE_ATTACK)
	e6:SetProperty(EFFECT_FLAG_IGNORE_IMMUNE+EFFECT_FLAG_SINGLE_RANGE)
	e6:SetRange(LOCATION_MZONE)
	e6:SetCondition(c700000002.imcon)
	c:RegisterEffect(e6)
	--[[tribute check
	local ea=Effect.CreateEffect(c)
	ea:SetType(EFFECT_TYPE_SINGLE)
	ea:SetCode(EFFECT_MATERIAL_CHECK)
	ea:SetValue(c700000002.valcheck)
	c:RegisterEffect(ea)
	--give atk effect only when summon
	local ex=Effect.CreateEffect(c)
	ex:SetType(EFFECT_TYPE_SINGLE)
	ex:SetCode(EFFECT_SUMMON_COST)
	ex:SetOperation(c700000002.facechk)
	ex:SetLabelObject(ea)
	c:RegisterEffect(ex)]]
	--Cannot be Set
	local e7=Effect.CreateEffect(c)
	e7:SetType(EFFECT_TYPE_FIELD)
	e7:SetCode(EFFECT_CANNOT_TURN_SET)
	e7:SetRange(LOCATION_MZONE)
	e7:SetTargetRange(LOCATION_MZONE,LOCATION_MZONE)
	e7:SetTarget(c700000002.tgset)
	c:RegisterEffect(e7)
	--Summon Cannot be Negated
	local e8=Effect.CreateEffect(c)
	e8:SetType(EFFECT_TYPE_SINGLE)
	e8:SetCode(EFFECT_CANNOT_DISABLE_SUMMON)
	e8:SetProperty(EFFECT_FLAG_CANNOT_DISABLE+EFFECT_FLAG_UNCOPYABLE)
	c:RegisterEffect(e8)
	--Summon Success: Effects of non-DIVINE monsters Cannot be Activated
	local e9=Effect.CreateEffect(c)
	e9:SetType(EFFECT_TYPE_SINGLE+EFFECT_TYPE_CONTINUOUS)
	e9:SetCode(EVENT_SUMMON_SUCCESS)
	e9:SetOperation(c700000002.sumsuc)
	c:RegisterEffect(e9)
    --Cannot be Tributed by Opponent
	local e16=Effect.CreateEffect(c)
	e16:SetType(EFFECT_TYPE_SINGLE)
	e16:SetProperty(EFFECT_FLAG_SINGLE_RANGE)
	e16:SetRange(LOCATION_MZONE)
	e16:SetCode(EFFECT_UNRELEASABLE_SUM)
	e16:SetValue(c700000002.tgvalue)
	c:RegisterEffect(e16)
    --Cannot Switch Controller
	local e17=Effect.CreateEffect(c)
	e17:SetType(EFFECT_TYPE_SINGLE)
	e17:SetProperty(EFFECT_FLAG_SINGLE_RANGE)
	e17:SetRange(LOCATION_MZONE)
	e17:SetCode(EFFECT_CANNOT_CHANGE_CONTROL)
	c:RegisterEffect(e17)
	--Cannot be Target
	local e10=Effect.CreateEffect(c)
	e10:SetType(EFFECT_TYPE_SINGLE)
	e10:SetCode(EFFECT_CANNOT_BE_EFFECT_TARGET)
	e10:SetProperty(EFFECT_FLAG_SINGLE_RANGE)
	e10:SetRange(LOCATION_MZONE)
	e10:SetValue(1)
	c:RegisterEffect(e10)
    --Cannot be Destroyed by Card Effect
	local e11=Effect.CreateEffect(c)
	e11:SetType(EFFECT_TYPE_SINGLE)
	e11:SetProperty(EFFECT_FLAG_SINGLE_RANGE)
	e11:SetRange(LOCATION_MZONE)
	e11:SetCode(EFFECT_INDESTRUCTABLE_EFFECT)
	e11:SetValue(1)
	c:RegisterEffect(e11)
	--Cannot be Removed
	local e12=Effect.CreateEffect(c)
	e12:SetType(EFFECT_TYPE_SINGLE)
	e12:SetProperty(EFFECT_FLAG_SINGLE_RANGE)
	e12:SetRange(LOCATION_MZONE)
	e12:SetCode(EFFECT_CANNOT_REMOVE)
	c:RegisterEffect(e12)
	--Cannot Send to Grave
	local e13=Effect.CreateEffect(c)
	e13:SetType(EFFECT_TYPE_SINGLE)
	e13:SetProperty(EFFECT_FLAG_SINGLE_RANGE)
	e13:SetRange(LOCATION_MZONE)
	e13:SetCode(EFFECT_CANNOT_TO_GRAVE)
	c:RegisterEffect(e13)
	--Cannot Return to Hand
	local e14=Effect.CreateEffect(c)
	e14:SetType(EFFECT_TYPE_SINGLE)
	e14:SetProperty(EFFECT_FLAG_SINGLE_RANGE)
	e14:SetRange(LOCATION_MZONE)
	e14:SetCode(EFFECT_CANNOT_TO_HAND)
	c:RegisterEffect(e14)
	--Cannot Return to Deck
	local e15=Effect.CreateEffect(c)
	e15:SetType(EFFECT_TYPE_SINGLE)
	e15:SetProperty(EFFECT_FLAG_SINGLE_RANGE)
	e15:SetRange(LOCATION_MZONE)
	e15:SetCode(EFFECT_CANNOT_TO_DECK)
	c:RegisterEffect(e15)
	--chant
	local sum1=Effect.CreateEffect(c)
	sum1:SetType(EFFECT_TYPE_SINGLE+EFFECT_TYPE_CONTINUOUS)
	sum1:SetCode(EVENT_SUMMON_SUCCESS)
	sum1:SetOperation(c700000002.sumop)
	c:RegisterEffect(sum1)
	local sum2=sum1:Clone()
	sum2:SetCode(EVENT_FLIP_SUMMON_SUCCESS)
	c:RegisterEffect(sum2)
	local sum3=sum1:Clone()
	sum3:SetCode(EVENT_SPSUMMON_SUCCESS)
	c:RegisterEffect(sum3)
	local sum=Effect.CreateEffect(c)
	sum:SetType(EFFECT_TYPE_SINGLE)
	sum:SetCode(700000002)
	sum:SetProperty(EFFECT_FLAG_CANNOT_DISABLE+EFFECT_FLAG_UNCOPYABLE+EFFECT_FLAG_IGNORE_IMMUNE)
	c:RegisterEffect(sum)
	if not c700000002.global_check then
		c700000002.global_check=true
	--register
		local ge1=Effect.CreateEffect(c)
		ge1:SetType(EFFECT_TYPE_FIELD+EFFECT_TYPE_CONTINUOUS)
		ge1:SetCode(EVENT_ADJUST)
		ge1:SetCountLimit(1)
		ge1:SetProperty(EFFECT_FLAG_NO_TURN_RESET)
		ge1:SetOperation(c700000002.chk)
		Duel.RegisterEffect(ge1,0)
	--De-Fusion
		local ge2=Effect.CreateEffect(c)
		ge2:SetType(EFFECT_TYPE_FIELD+EFFECT_TYPE_CONTINUOUS)
		ge2:SetCode(EVENT_ADJUST)
		ge2:SetOperation(c700000002.dfop)
		Duel.RegisterEffect(ge2,0)
	end
	--Change Battle Target when Special Summoned in Defense Position
	local e19=Effect.CreateEffect(c)
	e19:SetDescription(aux.Stringid(700000002,0))
	e19:SetProperty(EFFECT_FLAG_NO_TURN_RESET)
	e19:SetType(EFFECT_TYPE_FIELD+EFFECT_TYPE_TRIGGER_F)
	e19:SetCode(EVENT_BE_BATTLE_TARGET)
	e19:SetRange(LOCATION_MZONE)
	e19:SetCountLimit(1)
	e19:SetCondition(c700000002.cbtcon)
	e19:SetOperation(c700000002.cbtop)
	c:RegisterEffect(e19)
	--Cannot be Tributed
	local e26=Effect.CreateEffect(c)
	e26:SetType(EFFECT_TYPE_SINGLE)
	e26:SetProperty(EFFECT_FLAG_SINGLE_RANGE)
	e26:SetRange(LOCATION_MZONE)
	e26:SetCode(EFFECT_UNRELEASABLE_SUM)
	e26:SetValue(1)
	c:RegisterEffect(e26)
	local e27=e26:Clone()
	e27:SetCode(EFFECT_UNRELEASABLE_NONSUM)
	c:RegisterEffect(e27)
	--If Special Summoned: Send to Grave
	local e28=Effect.CreateEffect(c)
	e28:SetDescription(aux.Stringid(700000001,0))
	e28:SetCategory(CATEGORY_TOGRAVE)
	e28:SetType(EFFECT_TYPE_FIELD+EFFECT_TYPE_TRIGGER_F)
	e28:SetRange(LOCATION_MZONE)
	e28:SetProperty(EFFECT_FLAG_REPEAT+EFFECT_FLAG_CANNOT_DISABLE+EFFECT_FLAG_UNCOPYABLE)
	e28:SetCountLimit(1)
	e28:SetCode(EVENT_PHASE+PHASE_END)
	e28:SetCondition(c700000002.stgcon)
	e28:SetTarget(c700000002.stgtg)
	e28:SetOperation(c700000002.stgop)
	c:RegisterEffect(e28)
	--Unaffected by Trap Cards and non DIVINE Divine-Beast-Type monsters.
	local e29=Effect.CreateEffect(c)
	e29:SetType(EFFECT_TYPE_SINGLE)
	e29:SetProperty(EFFECT_FLAG_SINGLE_RANGE+EFFECT_FLAG_CANNOT_DISABLE+EFFECT_FLAG_UNCOPYABLE)
	e29:SetRange(LOCATION_MZONE)
	e29:SetCode(EFFECT_IMMUNE_EFFECT)
	e29:SetValue(c700000002.unaffectedval)
	c:RegisterEffect(e29)
end
function c700000002.chk(e,tp,eg,ep,ev,re,r,rp)
	if Duel.GetFlagEffect(tp,421)==0 and Duel.GetFlagEffect(1-tp,421)==0 then
		Duel.CreateToken(tp,421)
		Duel.CreateToken(1-tp,421)
		Duel.RegisterFlagEffect(tp,421,nil,0,1)
		Duel.RegisterFlagEffect(1-tp,421,nil,0,1)
	end
end
-------------------------------------------
--De-Fusion
function c700000002.dfop(e,tp,eg,ep,ev,re,r,rp)
	local c=e:GetHandler()  
	local g=Duel.GetMatchingGroup(Card.IsCode,c:GetControler(),0xff,0xff,nil,95286165)
	local tc=g:GetFirst()
	while tc do
		if tc:GetFlagEffect(700000002)==0 then
			--Activate
			local e1=Effect.CreateEffect(tc)
			e1:SetCategory(CATEGORY_SPECIAL_SUMMON)
			e1:SetDescription(aux.Stringid(4012,8))
			e1:SetCategory(CATEGORY_ATKCHANGE+CATEGORY_DEFCHANGE+CATEGORY_RECOVER)
			e1:SetType(EFFECT_TYPE_ACTIVATE)
			e1:SetCode(EVENT_FREE_CHAIN)
			e1:SetProperty(EFFECT_FLAG_CARD_TARGET+EFFECT_FLAG_DAMAGE_STEP+EFFECT_FLAG_IGNORE_IMMUNE)
			e1:SetHintTiming(0,TIMING_DRAW_PHASE+TIMING_END_PHASE)
			e1:SetTarget(c700000002.tg)
			e1:SetOperation(c700000002.op)
			tc:RegisterEffect(e1)
			tc:RegisterFlagEffect(700000002,0,0,1)   
		end
		tc=g:GetNext()
	end
end
function c700000002.dffilter2(c)
	return c:IsFaceup() and c:IsType(TYPE_FUSION) and (c:IsCode(10000010) or c:IsCode(511000237))
end
function c700000002.tg(e,tp,eg,ep,ev,re,r,rp,chk,chkc)
	if chkc then return chkc:IsLocation(LOCATION_MZONE) and c700000002.dffilter2(chkc) end
	if chk==0 then return Duel.IsExistingTarget(c700000002.dffilter2,tp,LOCATION_MZONE,LOCATION_MZONE,1,nil) end
	Duel.Hint(HINT_SELECTMSG,tp,HINTMSG_FACEUP)
	local g=Duel.SelectTarget(tp,c700000002.dffilter2,tp,LOCATION_MZONE,LOCATION_MZONE,1,1,nil)
	Duel.SetOperationInfo(0,CATEGORY_RECOVER,nil,0,tp,g:GetFirst():GetAttack())
end
function c700000002.op(e,tp,eg,ep,ev,re,r,rp)
	local c=e:GetHandler()
	local tc=Duel.GetFirstTarget()
	if not (tc:IsRelateToEffect(e) and tc:IsFaceup()) then return end
	local atk=tc:GetAttack()
	tc:ResetEffect(RESET_LEAVE,RESET_EVENT)
	Duel.Recover(tp,atk,REASON_EFFECT)
	tc:RegisterFlagEffect(236,RESET_EVENT+0x1fe0000,0,1)
	if Duel.GetAttacker() and Duel.GetAttacker()==tc then Duel.NegateAttack() end
	if Duel.GetCurrentChain()<=1 then return end
	for i=1,Duel.GetCurrentChain()-1 do
		local te=Duel.GetChainInfo(i,CHAININFO_TRIGGERING_EFFECT)
		local g=Duel.GetChainInfo(i,CHAININFO_TARGET_CARDS)
		if te:IsHasProperty(EFFECT_FLAG_CARD_TARGET) and g:IsContains(tc) then
			local rg=Group.CreateGroup()
			rg:Merge(g)
			rg:RemoveCard(tc)
			Duel.ChangeTargetCard(i,rg)
		end
	end
end
-------------------------------------------
function c700000002.sumoncon(e,c)
	if c==nil then return true end
	return Duel.GetLocationCount(c:GetControler(),LOCATION_MZONE)>-3 and Duel.GetTributeCount(c)>=3
end
function c700000002.sumonop(e,tp,eg,ep,ev,re,r,rp,c)
	local g=Duel.SelectTribute(tp,c,3,3)
	c:SetMaterial(g)
	Duel.Release(g,REASON_SUMMON+REASON_MATERIAL)
end
function c700000002.setcon(e,c)
	if not c then return true end
	return false
end
function c700000002.imcon(e)
	return e:GetHandler():IsSummonType(SUMMON_TYPE_SPECIAL)
end
-------------------------------------------
function c700000002.egpcon(e,tp,eg,ep,ev,re,r,rp)
	return e:GetHandler():IsPreviousLocation(LOCATION_GRAVE)
end
function c700000002.immortal(e,tp,eg,ep,ev,re,r,rp,chk)
	if chk==0 then return true end
	if c700000002.payatkcost(e,tp,eg,ep,ev,re,r,rp,0) then
		local op=Duel.SelectOption(tp,aux.Stringid(4012,3),aux.Stringid(4012,4),aux.Stringid(4012,5))
		if op==0 then
			c700000002.payatkcost(e,tp,eg,ep,ev,re,r,rp,1)
			e:SetOperation(c700000002.payatkop)
		elseif op==1 then
			e:SetOperation(c700000002.egpop)
		else
			e:SetOperation(nil)
		end
	else
		local op=Duel.SelectOption(tp,aux.Stringid(4012,4),aux.Stringid(4012,5))
		if op==0 then
			e:SetOperation(c700000002.egpop)
		else
			e:SetOperation(nil)
		end
	end
end
-------------------------------------------
--Point to Point Transfer
function c700000002.payatkcost(e,tp,eg,ep,ev,re,r,rp,chk)
	if chk==0 then return Duel.CheckLPCost(tp,2) end
	local lp=Duel.GetLP(tp)
	e:SetLabel(lp-1)
	Duel.PayLPCost(tp,lp-1)
end
function c700000002.payatkop(e,tp,eg,ep,ev,re,r,rp)
	local c=e:GetHandler()
	local lp=e:GetLabel()
	if c:IsFaceup() then
		local e1=Effect.CreateEffect(c)
		e1:SetType(EFFECT_TYPE_SINGLE)
		e1:SetCode(EFFECT_SET_BASE_ATTACK)
		e1:SetProperty(EFFECT_FLAG_SINGLE_RANGE+EFFECT_FLAG_IGNORE_IMMUNE)
		e1:SetRange(LOCATION_MZONE)
		e1:SetValue(c:GetBaseAttack()+lp)
		e1:SetReset(RESET_EVENT+0x1ff0000+RESET_PHASE+PHASE_END)
		c:RegisterEffect(e1)
		local e2=e1:Clone()
		e2:SetCode(EFFECT_SET_BASE_DEFENSE)
		e2:SetValue(c:GetBaseDefense()+lp)
		c:RegisterEffect(e2)
		local e3=Effect.CreateEffect(c)
		e3:SetType(EFFECT_TYPE_SINGLE)
		e3:SetCode(EFFECT_ADD_TYPE)
		e3:SetValue(TYPE_FUSION)
		e3:SetProperty(EFFECT_FLAG_CANNOT_DISABLE+EFFECT_FLAG_IGNORE_IMMUNE)
		e3:SetReset(RESET_EVENT+0x1fe0000+RESET_PHASE+PHASE_END)
		c:RegisterEffect(e3)
		local def=Effect.CreateEffect(c)
		def:SetReset(RESET_EVENT+0x1fe0000+RESET_PHASE+PHASE_END)
		c:RegisterEffect(def)
		local e4=Effect.CreateEffect(c)
		e4:SetDescription(aux.Stringid(4012,6))
		e4:SetType(EFFECT_TYPE_QUICK_O)
		e4:SetCode(EVENT_FREE_CHAIN)
		e4:SetRange(LOCATION_MZONE)
		e4:SetCost(c700000002.tatkcost)
		e4:SetOperation(c700000002.tatkop)
		e4:SetLabelObject(def)
		e4:SetReset(RESET_EVENT+0x1fe0000+RESET_PHASE+PHASE_END)
		c:RegisterEffect(e4)
		--gain atk/def
		local e5=Effect.CreateEffect(c)
		e5:SetType(EFFECT_TYPE_FIELD+EFFECT_TYPE_CONTINUOUS)
		e5:SetCode(EVENT_RECOVER)
		e5:SetRange(LOCATION_MZONE)
		e5:SetOperation(c700000002.atkop1)
		e5:SetReset(RESET_EVENT+0x1fe0000+RESET_PHASE+PHASE_END)
		c:RegisterEffect(e5)
		--gain atk/def
		local e6=Effect.CreateEffect(c)
		e6:SetType(EFFECT_TYPE_FIELD+EFFECT_TYPE_CONTINUOUS)
		e6:SetCode(EVENT_CHAIN_END)
		e6:SetRange(LOCATION_MZONE)
		e6:SetOperation(c700000002.atkop2)
		e6:SetReset(RESET_EVENT+0x1fe0000+RESET_PHASE+PHASE_END)
		c:RegisterEffect(e6)
		local e7=Effect.CreateEffect(c)
		e7:SetType(EFFECT_TYPE_FIELD)
		e7:SetRange(LOCATION_MZONE)
		e7:SetTargetRange(0,LOCATION_MZONE)
		e7:SetProperty(EFFECT_FLAG_IGNORE_IMMUNE+EFFECT_FLAG_CANNOT_DISABLE)
		e7:SetCode(EFFECT_CANNOT_BE_BATTLE_TARGET)
		e7:SetTarget(c700000002.valtg)
		e7:SetValue(c700000002.vala)
		e7:SetReset(RESET_EVENT+0x1fe0000+RESET_PHASE+PHASE_END)
		c:RegisterEffect(e7)
		local e8=Effect.CreateEffect(c)
		e8:SetType(EFFECT_TYPE_SINGLE+EFFECT_TYPE_CONTINUOUS)
		e8:SetProperty(EFFECT_FLAG_IGNORE_IMMUNE+EFFECT_FLAG_CANNOT_DISABLE)
		e8:SetCode(EVENT_DAMAGE_STEP_END)
		e8:SetCondition(c700000002.dircon)
		e8:SetOperation(c700000002.dirop)
		e8:SetReset(RESET_EVENT+0x1fe0000+RESET_PHASE+PHASE_END)
		c:RegisterEffect(e8)
		local e9=Effect.CreateEffect(c)
		e9:SetType(EFFECT_TYPE_SINGLE)
		e9:SetCode(EFFECT_EXTRA_ATTACK)
		e9:SetValue(9999)
		e9:SetReset(RESET_EVENT+0x1fe0000+RESET_PHASE+PHASE_END)
		c:RegisterEffect(e9)
		local e10=Effect.CreateEffect(c)
		e10:SetType(EFFECT_TYPE_SINGLE+EFFECT_TYPE_CONTINUOUS)
		e10:SetCode(EVENT_DAMAGE_STEP_END)
		e10:SetProperty(EFFECT_FLAG_CANNOT_DISABLE)
		e10:SetCondition(c700000002.uncon)
		e10:SetOperation(c700000002.unop)
		e10:SetReset(RESET_EVENT+0x1fe0000+RESET_PHASE+PHASE_END)
		c:RegisterEffect(e10)
	end
end
function c700000002.atkop1(e,tp,eg,ep,ev,re,r,rp)
	if ep~=tp then return end
	local e1=Effect.CreateEffect(e:GetHandler())
	e1:SetType(EFFECT_TYPE_SINGLE)
	e1:SetProperty(EFFECT_FLAG_COPY_INHERIT+EFFECT_FLAG_IGNORE_IMMUNE)
	e1:SetCode(EFFECT_UPDATE_ATTACK)
	e1:SetValue(ev)
	e1:SetReset(RESET_EVENT+0x1ff0000)
	e:GetHandler():RegisterEffect(e1)
	local e1=Effect.CreateEffect(e:GetHandler())
	e1:SetType(EFFECT_TYPE_SINGLE)
	e1:SetProperty(EFFECT_FLAG_COPY_INHERIT+EFFECT_FLAG_IGNORE_IMMUNE)
	e1:SetCode(EFFECT_UPDATE_DEFENSE)
	e1:SetValue(ev)
	e1:SetReset(RESET_EVENT+0x1ff0000)
	e:GetHandler():RegisterEffect(e1)
	Duel.SetLP(tp,1,REASON_EFFECT)
end
function c700000002.atkop2(e,tp,eg,ep,ev,re,r,rp)
	local lp=Duel.GetLP(tp)
	if lp<=1 then return end
	local e1=Effect.CreateEffect(e:GetHandler())
	e1:SetType(EFFECT_TYPE_SINGLE)
	e1:SetProperty(EFFECT_FLAG_COPY_INHERIT+EFFECT_FLAG_IGNORE_IMMUNE)
	e1:SetCode(EFFECT_UPDATE_ATTACK)
	e1:SetValue(lp-1)
	e1:SetReset(RESET_EVENT+0x1ff0000)
	e:GetHandler():RegisterEffect(e1)
	local e1=Effect.CreateEffect(e:GetHandler())
	e1:SetType(EFFECT_TYPE_SINGLE)
	e1:SetProperty(EFFECT_FLAG_COPY_INHERIT+EFFECT_FLAG_IGNORE_IMMUNE)
	e1:SetCode(EFFECT_UPDATE_DEFENSE)
	e1:SetValue(lp-1)
	e1:SetReset(RESET_EVENT+0x1ff0000)
	e:GetHandler():RegisterEffect(e1)
	Duel.SetLP(tp,1,REASON_EFFECT)
end
function c700000002.tatkcost(e,tp,eg,ep,ev,re,r,rp,chk)
	if chk==0 then return Duel.CheckReleaseGroup(tp,nil,1,e:GetHandler()) end
	local g=Duel.SelectReleaseGroup(tp,nil,1,10,e:GetHandler())
	local tc=g:GetFirst()
	local suma=0
	local sumd=0
	while tc do
		suma=suma+tc:GetAttack()
		sumd=sumd+tc:GetDefense()
		tc=g:GetNext()
	end
	e:SetLabel(suma)
	e:GetLabelObject():SetLabel(sumd)
	Duel.Release(g,REASON_COST)
end
function c700000002.tatkop(e,tp,eg,ep,ev,re,r,rp)
	local atk=e:GetLabel()
	local def=e:GetLabelObject():GetLabel()
	local c=e:GetHandler()
	if c:GetFlagEffect(236)>0 then return end
	local e1=Effect.CreateEffect(c)
	e1:SetType(EFFECT_TYPE_SINGLE)
	e1:SetCode(EFFECT_UPDATE_ATTACK)
	e1:SetValue(atk)
	e1:SetReset(RESET_EVENT+0x1fe0000)
	c:RegisterEffect(e1)
	local e2=Effect.CreateEffect(c)
	e2:SetType(EFFECT_TYPE_SINGLE)
	e2:SetCode(EFFECT_UPDATE_DEFENSE)
	e2:SetValue(def)
	e2:SetReset(RESET_EVENT+0x1fe0000)
	c:RegisterEffect(e2)
end
function c700000002.uncon(e,tp,eg,ep,ev,re,r,rp)
	local bc=e:GetHandler():GetBattleTarget()
	return bc and bc:IsRelateToBattle()
end
function c700000002.unop(e,tp,eg,ep,ev,re,r,rp)
	local bc=e:GetHandler():GetBattleTarget()
	bc:RegisterFlagEffect(511000237,RESET_EVENT+0x1fe0000+RESET_PHASE+PHASE_BATTLE,0,1)
end
function c700000002.valtg(e,c)
	return c:GetFlagEffect(511000237)>0
end
function c700000002.vala(e,c)
	return c==e:GetHandler()
end
function c700000002.dircon(e,tp,eg,ep,ev,re,r,rp)
	return Duel.GetAttackTarget()==nil and Duel.GetAttacker()==e:GetHandler()
end
function c700000002.dirop(e,tp,eg,ep,ev,re,r,rp)
	local e1=Effect.CreateEffect(e:GetHandler())
	e1:SetType(EFFECT_TYPE_SINGLE)
	e1:SetProperty(EFFECT_FLAG_IGNORE_IMMUNE+EFFECT_FLAG_CANNOT_DISABLE)
	e1:SetCode(EFFECT_CANNOT_DIRECT_ATTACK)
	e1:SetReset(RESET_EVENT+0x1fe0000+RESET_PHASE+PHASE_BATTLE)
	e:GetHandler():RegisterEffect(e1)
end
-------------------------------------------
--Egyption God Phoenix
function c700000002.egpop(e,tp,eg,ep,ev,re,r,rp)
	local c=e:GetHandler()
	if c:IsFaceup() then
		local e1=Effect.CreateEffect(c)
		e1:SetType(EFFECT_TYPE_SINGLE)
		e1:SetProperty(EFFECT_FLAG_CANNOT_DISABLE+EFFECT_FLAG_IGNORE_IMMUNE)
		e1:SetReset(RESET_EVENT+0x1fe1000+RESET_PHASE+PHASE_END)
		e1:SetCode(EFFECT_CHANGE_CODE)
		e1:SetValue(511000237)
		c:RegisterEffect(e1)
		local e2=Effect.CreateEffect(c)
		e2:SetDescription(aux.Stringid(4012,7))
		e2:SetCategory(CATEGORY_DESTROY)
		e2:SetType(EFFECT_TYPE_QUICK_O)
		e2:SetCode(EVENT_FREE_CHAIN)
		e2:SetProperty(EFFECT_FLAG_CARD_TARGET+EFFECT_FLAG_IGNORE_IMMUNE)
		e2:SetRange(LOCATION_MZONE)
		e2:SetCost(c700000002.descost)
		e2:SetTarget(c700000002.destg)
		e2:SetOperation(c700000002.desop)
		e2:SetReset(RESET_EVENT+0x1fe0000+RESET_PHASE+PHASE_END)
		c:RegisterEffect(e2)
		--immune
		local e3=Effect.CreateEffect(c)
		e3:SetType(EFFECT_TYPE_SINGLE)
		e3:SetCode(EFFECT_AVOID_BATTLE_DAMAGE)
		e3:SetProperty(EFFECT_FLAG_IGNORE_IMMUNE)
		e3:SetValue(1)
		e3:SetReset(RESET_EVENT+0x1fe0000+RESET_PHASE+PHASE_END)
		c:RegisterEffect(e3)
		local e4=e3:Clone()
		e4:SetCode(EFFECT_INDESTRUCTABLE)
		c:RegisterEffect(e4)
		local e5=e3:Clone()
		e5:SetCode(EFFECT_UNRELEASABLE_EFFECT)
		c:RegisterEffect(e5)
		local e6=e3:Clone()
		e6:SetCode(EFFECT_CANNOT_REMOVE)
		c:RegisterEffect(e6)
		local e7=e3:Clone()
		e7:SetCode(EFFECT_CANNOT_TO_HAND)
		c:RegisterEffect(e7)
		local e8=e3:Clone()
		e8:SetCode(EFFECT_CANNOT_TO_DECK)
		c:RegisterEffect(e8)
		local e9=e3:Clone()
		e9:SetCode(EFFECT_CANNOT_TO_GRAVE)
		c:RegisterEffect(e9)
		c:RegisterFlagEffect(511000237,RESET_EVENT+0x1fe0000+RESET_PHASE+PHASE_END,0,1)
	end
end
function c700000002.descost(e,tp,eg,ep,ev,re,r,rp,chk)
	if chk==0 then return Duel.CheckLPCost(tp,1000) end
	Duel.PayLPCost(tp,1000)
end
function c700000002.destg(e,tp,eg,ep,ev,re,r,rp,chk,chkc)
	if chkc then return chkc:IsLocation(LOCATION_MZONE) and chkc:IsControler(1-tp) and chkc~=e:GetHandler() end
	if chk==0 then return Duel.IsExistingTarget(nil,tp,LOCATION_MZONE,LOCATION_MZONE,1,e:GetHandler(),e) end
	Duel.Hint(HINT_SELECTMSG,tp,HINTMSG_DESTROY)
	local g=Duel.SelectTarget(tp,nil,tp,LOCATION_MZONE,LOCATION_MZONE,1,1,e:GetHandler(),e)
	Duel.SetOperationInfo(0,CATEGORY_DESTROY,g,1,0,0)
end
function c700000002.desop(e,tp,eg,ep,ev,re,r,rp)
	local c=e:GetHandler()
	local tc=Duel.GetFirstTarget()
	if tc and tc:IsRelateToEffect(e) and c:GetFlagEffect(511000237)>0 then
		local e1=Effect.CreateEffect(c)
		e1:SetType(EFFECT_TYPE_SINGLE)
		e1:SetCode(EFFECT_DISABLE)
		e1:SetProperty(EFFECT_FLAG_IGNORE_IMMUNE)
		e1:SetReset(RESET_EVENT+0x1fe0000)
		tc:RegisterEffect(e1)
		local e2=Effect.CreateEffect(c)
		e2:SetType(EFFECT_TYPE_SINGLE)
		e2:SetCode(EFFECT_DISABLE_EFFECT)
		e2:SetProperty(EFFECT_FLAG_IGNORE_IMMUNE)
		e2:SetReset(RESET_EVENT+0x1fe0000)
		tc:RegisterEffect(e2)
		local e3=Effect.CreateEffect(c)
		e3:SetType(EFFECT_TYPE_SINGLE)
		e3:SetRange(LOCATION_MZONE)
		e3:SetCode(EFFECT_IMMUNE_EFFECT)
		e3:SetProperty(EFFECT_FLAG_IGNORE_IMMUNE)
		e3:SetValue(c700000002.desfil)
		e3:SetReset(RESET_EVENT+0x1fe0000)
		tc:RegisterEffect(e3)
		Duel.BreakEffect()
	end
	Duel.Destroy(tc,REASON_EFFECT)
end
function c700000002.desfil(e,te)
	return te:GetOwner()~=e:GetOwner()
end
function c700000002.kek(e)
	return true
end
function c700000002.sumop(e,tp,eg,ep,ev,re,r,rp)
	local c=e:GetHandler()
--[[	local chant=true
	Duel.Hint(HINT_SELECTMSG,tp,aux.Stringid(4012,9))
	local op=Duel.SelectOption(tp,aux.Stringid(4012,10),aux.Stringid(4012,11),aux.Stringid(4012,12),aux.Stringid(4012,13),aux.Stringid(4012,14),aux.Stringid(4012,15))
	if op~=3 then
		local op2=Duel.SelectOption(1-tp,aux.Stringid(4012,10),aux.Stringid(4012,11),aux.Stringid(4012,12),aux.Stringid(4012,13),aux.Stringid(4012,14),aux.Stringid(4012,15))
		if op2~=3 then
			chant=false
		else
			if not Duel.GetControl(c,1-tp) then
				chant=false
			end
		end
	end
	if chant==false then
		local e1=Effect.CreateEffect(c)
		e1:SetType(EFFECT_TYPE_SINGLE)
		e1:SetCode(EFFECT_CANNOT_ATTACK)
		e1:SetProperty(EFFECT_FLAG_IGNORE_IMMUNE)
		e1:SetReset(RESET_EVENT+0x1fe0000)
		c:RegisterEffect(e1)
		local e2=e1:Clone()
		e2:SetCode(EFFECT_CANNOT_TRIGGER)
		c:RegisterEffect(e2)
		local e3=e1:Clone()
		e3:SetProperty(EFFECT_FLAG_SINGLE_RANGE+EFFECT_FLAG_IGNORE_IMMUNE)
		e3:SetCode(EFFECT_IGNORE_BATTLE_TARGET)
		e3:SetRange(LOCATION_MZONE)
		e3:SetCondition(c700000002.kek)
		c:RegisterEffect(e3)
	end
	Duel.BreakEffect()]]
	--control
	local e6=Effect.CreateEffect(c)
	e6:SetType(EFFECT_TYPE_SINGLE)
	e6:SetProperty(EFFECT_FLAG_SINGLE_RANGE+EFFECT_FLAG_CANNOT_DISABLE+EFFECT_FLAG_UNCOPYABLE+EFFECT_FLAG_IGNORE_IMMUNE)
	e6:SetRange(LOCATION_MZONE)
	e6:SetCode(EFFECT_CANNOT_CHANGE_CONTROL)
	e6:SetReset(RESET_EVENT+0x1fe0000)
	c:RegisterEffect(e6)
	if e:GetCode()~=EVENT_SUMMON_SUCCESS then return end
	local g=c:GetMaterial()
	local tc=g:GetFirst()
	local atk=0
	local def=0
	while tc do
		local catk=tc:GetPreviousAttackOnField()
		local cdef=tc:GetPreviousDefenseOnField()
		atk=atk+(catk>=0 and catk or 0)
		def=def+(cdef>=0 and cdef or 0)
		tc=g:GetNext()
	end
	local e4=Effect.CreateEffect(c)
	e4:SetType(EFFECT_TYPE_SINGLE)
	e4:SetCode(EFFECT_SET_BASE_ATTACK)
	e4:SetProperty(EFFECT_FLAG_SINGLE_RANGE)
	e4:SetRange(LOCATION_MZONE)
	e4:SetValue(atk)
	e4:SetReset(RESET_EVENT+0xff0000)
	c:RegisterEffect(e4)
	local e5=e4:Clone()
	e5:SetCode(EFFECT_SET_BASE_DEFENSE)
	e5:SetValue(def)
	c:RegisterEffect(e5)
end
-------------------------------------------
--redundant
function c700000002.valcheck(e,c)
	local g=c:GetMaterial()
	local tc=g:GetFirst()
	local atk=0
	local def=0
	while tc do
		local catk=tc:GetTextAttack()
		local cdef=tc:GetTextDefense()
		atk=atk+(catk>=0 and catk or 0)
		def=def+(cdef>=0 and cdef or 0)
		tc=g:GetNext()
	end
	if e:GetLabel()==1 then
		e:SetLabel(0)
		local e1=Effect.CreateEffect(c)
		e1:SetType(EFFECT_TYPE_SINGLE)
		e1:SetCode(EFFECT_SET_BASE_ATTACK)
		e1:SetProperty(EFFECT_FLAG_SINGLE_RANGE)
		e1:SetRange(LOCATION_MZONE)
		e1:SetValue(atk)
		e1:SetReset(RESET_EVENT+0xff0000)
		c:RegisterEffect(e1)
		local e2=e1:Clone()
		e2:SetCode(EFFECT_SET_BASE_DEFENSE)
		e2:SetValue(def)
		c:RegisterEffect(e2)
	end
end
function c700000002.facechk(e,tp,eg,ep,ev,re,r,rp)
	e:GetLabelObject():SetLabel(1)
end
function c700000002.cbtcon(e,tp,eg,ep,ev,re,r,rp)
	local c=e:GetHandler()
	local bt=Duel.GetAttackTarget()
	return c~=bt and bt:IsControler(tp) and bit.band(e:GetHandler():GetSummonType(),SUMMON_TYPE_SPECIAL)==SUMMON_TYPE_SPECIAL and e:GetHandler():IsDefensePos()
end
function c700000002.cbtop(e,tp,eg,ep,ev,re,r,rp)
	Duel.ChangeAttackTarget(e:GetHandler())
end
function c700000002.tgset(e,c)
return c:IsAttribute(ATTRIBUTE_DEVINE)
end
function c700000002.sumsuc(e,tp,eg,ep,ev,re,r,rp)
	local c=e:GetHandler()
	local e1=Effect.CreateEffect(c)
	e1:SetType(EFFECT_TYPE_FIELD)
	e1:SetCode(EFFECT_CANNOT_TRIGGER)
	e1:SetRange(LOCATION_MZONE)
	e1:SetTargetRange(LOCATION_GRAVE,LOCATION_GRAVE+LOCATION_MZONE,LOCATION_MZONE+LOCATION_SZONE,LOCATION_SZONE+LOCATION_HAND,LOCATION_HAND)
	e1:SetTarget(c700000002.triggertg)
	e1:SetReset(RESET_EVENT+EVENT_SUMMON_SUCCESS)
	c:RegisterEffect(e1)
end
function c700000002.tgvalue(e,re,rp)
local c=e:GetHandler()
 return not c:IsControler(tp)
end
function c700000002.stgcon(e,tp,eg,ep,ev,re,r,rp)
	return bit.band(e:GetHandler():GetSummonType(),SUMMON_TYPE_SPECIAL)==SUMMON_TYPE_SPECIAL and e:GetHandler():IsPreviousLocation(LOCATION_GRAVE)
end
function c700000002.stgtg(e,tp,eg,ep,ev,re,r,rp,chk)
	if chk==0 then return true end
	Duel.SetOperationInfo(0,CATEGORY_TOGRAVE,e:GetHandler(),1,0,0)
end
function c700000002.stgop(e,tp,eg,ep,ev,re,r,rp)
	local c=e:GetHandler()
	if c:IsRelateToEffect(e) and c:IsFaceup() then
		local e1=Effect.CreateEffect(c)
		e1:SetType(EFFECT_TYPE_SINGLE)
		e1:SetCode(EFFECT_DISABLE)
		e1:SetReset(RESET_EVENT+0x1fe0000+RESET_PHASE+PHASE_END)
		c:RegisterEffect(e1)
		local e2=Effect.CreateEffect(c)
		e2:SetType(EFFECT_TYPE_SINGLE)
		e2:SetCode(EFFECT_DISABLE_EFFECT)
		e2:SetReset(RESET_EVENT+0x1fe0000+RESET_PHASE+PHASE_END)
		c:RegisterEffect(e2)
		Duel.BreakEffect()
		Duel.SendtoGrave(c,REASON_EFFECT)
	end
end
function c700000002.unaffectedval(e,te)
	local c=te:GetHandler()
	return not te:IsActiveType(TYPE_SPELL) and not c:IsAttribute(ATTRIBUTE_DEVINE) and not c:IsCode(53569894)
end